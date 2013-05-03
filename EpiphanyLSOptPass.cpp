//===-- EpiphanyLoadStoreOptimizer.cpp - Epiphany load / store opt. pass ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that performs load / store related peephole
// optimizations. This pass should be run after register allocation.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "epiphany-ldst-opt"
#include "Epiphany.h"
#include "EpiphanyInstrInfo.h"
#include "EpiphanyRegisterInfo.h"
#include "EpiphanyMachineFunctionInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"
using namespace llvm;


STATISTIC(NumLdStMoved, "Number of load / store instructions moved");
STATISTIC(NumLDRDFormed,"Number of ldrd created before allocation");
STATISTIC(NumSTRDFormed,"Number of strd created before allocation");


/// isMemoryOp - Returns true if instruction is a memory operation that this
/// pass is capable of operating on.
static bool isMemoryOp(const MachineInstr *MI) {
  // When no memory operands are present, conservatively assume unaligned,
  // volatile, unfoldable.
  if (!MI->hasOneMemOperand())
    return false;

  const MachineMemOperand *MMO = *MI->memoperands_begin();

  // Don't touch volatile memory accesses - we may be changing their order.
  if (MMO->isVolatile())
    return false;

  // Unaligned ldr/str is emulated by some kernels, but unaligned ldm/stm is
  // not.
  if (MMO->getAlignment() < 4)
    return false;

  // str <undef> could probably be eliminated entirely, but for now we just want
  // to avoid making a mess of it.
  // FIXME: Use str <undef> as a wildcard to enable better stm folding.
  if (MI->getNumOperands() > 0 && MI->getOperand(0).isReg() &&
      MI->getOperand(0).isUndef())
    return false;

  // Likewise don't mess with references to undefined addresses.
  if (MI->getNumOperands() > 1 && MI->getOperand(1).isReg() &&
      MI->getOperand(1).isUndef())
    return false;

  int Opcode = MI->getOpcode();
  switch (Opcode) {
  default: break;
  case Epiphany::LS32_LDR:
  case Epiphany::LSFP32_LDR:
  case Epiphany::LS32_STR:
  case Epiphany::LSFP32_STR:
    return MI->getOperand(1).isReg();
  }
  return false;
}

static int getMemoryOpOffset(const MachineInstr *MI) {
  int Opcode = MI->getOpcode();
  unsigned NumOperands = MI->getDesc().getNumOperands();
  return MI->getOperand(NumOperands-1).getImm() * 4;//only 32bit
}

/// EpiphanyPreAllocLoadStoreOpt - Pre- register allocation pass that move
/// load / stores from consecutive locations close to make it more
/// likely they will be combined later.

namespace {
  struct EpiphanyPreAllocLoadStoreOpt : public MachineFunctionPass{
    static char ID;
    EpiphanyPreAllocLoadStoreOpt() : MachineFunctionPass(ID) {}

    const DataLayout *TD;
    const TargetInstrInfo *TII;
    const TargetRegisterInfo *TRI;
    MachineRegisterInfo *MRI;
    MachineFunction *MF;

    virtual bool runOnMachineFunction(MachineFunction &Fn);

    virtual const char *getPassName() const {
      return "Epiphany pre- register allocation load / store optimization pass";
    }

  private:
    bool CanFormLdStDWord(MachineInstr *Op0, MachineInstr *Op1, DebugLoc &dl,
                          unsigned &NewOpc, unsigned &EvenReg,
                          unsigned &OddReg, unsigned &BaseReg,
                          int &Offset);
    bool RescheduleOps(MachineBasicBlock *MBB,
                       SmallVector<MachineInstr*, 4> &Ops,
                       unsigned Base, bool isLd,
                       DenseMap<MachineInstr*, unsigned> &MI2LocMap);
    bool RescheduleLoadStoreInstrs(MachineBasicBlock *MBB);
  };
  char EpiphanyPreAllocLoadStoreOpt::ID = 0;
}

bool EpiphanyPreAllocLoadStoreOpt::runOnMachineFunction(MachineFunction &Fn) {
  TD  = Fn.getTarget().getDataLayout();
  TII = Fn.getTarget().getInstrInfo();
  TRI = Fn.getTarget().getRegisterInfo();
  MRI = &Fn.getRegInfo();
  MF  = &Fn;

  bool Modified = false;
  for (MachineFunction::iterator MFI = Fn.begin(), E = Fn.end(); MFI != E;
       ++MFI)
    Modified |= RescheduleLoadStoreInstrs(MFI);

  return Modified;
}

static bool IsSafeAndProfitableToMove(bool isLd, unsigned Base,
                                      MachineBasicBlock::iterator I,
                                      MachineBasicBlock::iterator E,
                                      SmallPtrSet<MachineInstr*, 4> &MemOps,
                                      SmallSet<unsigned, 4> &MemRegs,
                                      const TargetRegisterInfo *TRI) {
  // Are there stores / loads / calls between them?
  // FIXME: This is overly conservative. We should make use of alias information
  // some day.
  SmallSet<unsigned, 4> AddedRegPressure;
  while (++I != E) {
    if (I->isDebugValue() || MemOps.count(&*I))
      continue;
    if (I->isCall() || I->isTerminator() || I->hasUnmodeledSideEffects())
      return false;
    if (isLd && I->mayStore())
      return false;
    if (!isLd) {
      if (I->mayLoad())
        return false;
      // It's not safe to move the first 'str' down.
      // str r1, [r0]
      // strh r5, [r0]
      // str r4, [r0, #+4]
      if (I->mayStore())
        return false;
    }
    for (unsigned j = 0, NumOps = I->getNumOperands(); j != NumOps; ++j) {
      MachineOperand &MO = I->getOperand(j);
      if (!MO.isReg())
        continue;
      unsigned Reg = MO.getReg();
      if (MO.isDef() && TRI->regsOverlap(Reg, Base))
        return false;
      if (Reg != Base && !MemRegs.count(Reg))
        AddedRegPressure.insert(Reg);
    }
  }

  // Estimate register pressure increase due to the transformation.
  if (MemRegs.size() <= 4)
    // Ok if we are moving small number of instructions.
    return true;
  return AddedRegPressure.size() <= MemRegs.size() * 2;
}


/// Copy Op0 and Op1 operands into a new array assigned to MI.
static void concatenateMemOperands(MachineInstr *MI, MachineInstr *Op0,
                                   MachineInstr *Op1) {
  assert(MI->memoperands_empty() && "expected a new machineinstr");
  size_t numMemRefs = (Op0->memoperands_end() - Op0->memoperands_begin())
    + (Op1->memoperands_end() - Op1->memoperands_begin());

  MachineFunction *MF = MI->getParent()->getParent();
  MachineSDNode::mmo_iterator MemBegin = MF->allocateMemRefsArray(numMemRefs);
  MachineSDNode::mmo_iterator MemEnd =
    std::copy(Op0->memoperands_begin(), Op0->memoperands_end(), MemBegin);
  MemEnd =
    std::copy(Op1->memoperands_begin(), Op1->memoperands_end(), MemEnd);
  MI->setMemRefs(MemBegin, MemEnd);
}

bool
EpiphanyPreAllocLoadStoreOpt::CanFormLdStDWord(MachineInstr *Op0, MachineInstr *Op1,
                                          DebugLoc &dl,
                                          unsigned &NewOpc, unsigned &EvenReg,
                                          unsigned &OddReg, unsigned &BaseReg,
                                          int &Offset) {
 
  unsigned Opcode = Op0->getOpcode();
  switch(Op0->getOpcode()){
	case Epiphany::LS32_LDR:
	case Epiphany::LSFP32_LDR: NewOpc = Epiphany::LSFP64_LDR;break;
	case Epiphany::LSFP32_STR:
	case Epiphany::LS32_STR: NewOpc = Epiphany::LSFP64_STR;break;
	default: return false;
  }

  // Make sure the base address satisfies i64 ld / st alignment requirement.
  if (!Op0->hasOneMemOperand() || !(*Op0->memoperands_begin())->getValue() || (*Op0->memoperands_begin())->isVolatile())
    return false;

  unsigned Align = (*Op0->memoperands_begin())->getAlignment();
  const Function *Func = MF->getFunction();
  if (Align < 8)
    return false;

  // Then make sure the immediate offset fits.
  Offset = getMemoryOpOffset(Op0);

  EvenReg = Op0->getOperand(0).getReg();
  OddReg  = Op1->getOperand(0).getReg();
  if (EvenReg == OddReg)
    return false;
  BaseReg = Op0->getOperand(1).getReg();
  dl = Op0->getDebugLoc();
  return true;
}

namespace {
  struct OffsetCompare {
    bool operator()(const MachineInstr *LHS, const MachineInstr *RHS) const {
      int LOffset = getMemoryOpOffset(LHS);
      int ROffset = getMemoryOpOffset(RHS);
      assert(LHS == RHS || LOffset != ROffset);
      return LOffset > ROffset;
    }
  };
}

bool EpiphanyPreAllocLoadStoreOpt::RescheduleOps(MachineBasicBlock *MBB,
	SmallVector<MachineInstr*, 4> &Ops,
	unsigned Base, bool isLd,
	DenseMap<MachineInstr*, unsigned> &MI2LocMap) {
		bool RetVal = false;

		// Sort by offset (in reverse order).
		std::sort(Ops.begin(), Ops.end(), OffsetCompare());

		// The loads / stores of the same base are in order. Scan them from first to
		// last and check for the following:
		// 1. Any def of base.
		// 2. Any gaps.
		while (Ops.size() > 1) {
			unsigned FirstLoc = ~0U;
			unsigned LastLoc = 0;
			MachineInstr *FirstOp = 0;
			MachineInstr *LastOp = 0;
			int LastOffset = 0;
			//unsigned LastOpcode = 0;
			unsigned LastBytes = 0;
			unsigned NumMove = 0;
			for (int i = Ops.size() - 1; i >= 0; --i) {
				MachineInstr *Op = Ops[i];
				unsigned Loc = MI2LocMap[Op];
				if (Loc <= FirstLoc) {
					FirstLoc = Loc;
					FirstOp = Op;
				}
				if (Loc >= LastLoc) {
					LastLoc = Loc;
					LastOp = Op;
				}

				//unsigned LSMOpcode = 0;//getLoadStoreMultipleOpcode(Op->getOpcode(), Epiphany_AM::ia);
				//if (LastOpcode && LSMOpcode != LastOpcode)
				//  break;

				int Offset = getMemoryOpOffset(Op);
				unsigned Bytes = 4;//getLSMultipleTransferSize(Op);
				if (LastBytes) {
					if (Bytes != LastBytes || Offset != (LastOffset + (int)Bytes))
						break;
				}
				LastOffset = Offset;
				LastBytes = Bytes;
				//LastOpcode = LSMOpcode;
				if (++NumMove == 8) // FIXME: Tune this limit.
					break;
			}

			if (NumMove <= 1)
				Ops.pop_back();
			else {
				SmallPtrSet<MachineInstr*, 4> MemOps;
				SmallSet<unsigned, 4> MemRegs;
				for (int i = NumMove-1; i >= 0; --i) {
					MemOps.insert(Ops[i]);
					MemRegs.insert(Ops[i]->getOperand(0).getReg());
				}

				// Be conservative, if the instructions are too far apart, don't
				// move them. We want to limit the increase of register pressure.
				bool DoMove = (LastLoc - FirstLoc) <= NumMove*4; // FIXME: Tune this.
				if (DoMove)
					DoMove = IsSafeAndProfitableToMove(isLd, Base, FirstOp, LastOp,
					MemOps, MemRegs, TRI);
				if (!DoMove) {
					for (unsigned i = 0; i != NumMove; ++i)
						Ops.pop_back();
				} else {
					// This is the new location for the loads / stores.
					MachineBasicBlock::iterator InsertPos = isLd ? FirstOp : LastOp;
					while (InsertPos != MBB->end() && (MemOps.count(InsertPos) || InsertPos->isDebugValue()))
						++InsertPos;

					// If we are moving a pair of loads / stores, see if it makes sense
					// to try to allocate a pair of registers that can form register pairs.
					MachineInstr *Op0 = Ops.back();
					MachineInstr *Op1 = Ops[Ops.size()-2];
					unsigned EvenReg = 0, OddReg = 0;
					unsigned BaseReg = 0, DoubleReg = 0;
					//bool isT2 = false;
					unsigned NewOpc = 0;
					int Offset = 0;
					DebugLoc dl;
					if (NumMove == 2 && CanFormLdStDWord(Op0, Op1, dl, NewOpc,
						EvenReg, OddReg, BaseReg,
						Offset)) {
							Ops.pop_back();
							Ops.pop_back();

							const MCInstrDesc &MCID = TII->get(NewOpc);
							const TargetRegisterClass *TRC = TII->getRegClass(MCID, 0, TRI, *MF);
							DoubleReg = MRI->createVirtualRegister(TRC);


							// Form the pair instruction.
							if (isLd) {
								MachineInstrBuilder MIB = BuildMI(*MBB, InsertPos, dl, MCID)
									.addReg(DoubleReg, RegState::Define)
									.addReg(BaseReg);
								MIB.addImm(Offset);
								concatenateMemOperands(MIB, Op0, Op1);
								DEBUG(dbgs() << "Formed " << *MIB << "\n");
								++NumLDRDFormed;

								//FIXME: i think this is kinda bad, use EXTRACT_SUBREG instead?

								// Collect all the uses of this MI's DPR def for updating later.
								SmallVector<MachineOperand*, 8> Uses0,Uses1;

								for (MachineRegisterInfo::use_iterator I = MRI->use_begin(Op0->getOperand(0).getReg()),E = MRI->use_end(); I != E; ++I)
									Uses0.push_back(&I.getOperand());

								for (MachineRegisterInfo::use_iterator I = MRI->use_begin(Op1->getOperand(0).getReg()),E = MRI->use_end(); I != E; ++I)
									Uses1.push_back(&I.getOperand());

								for (SmallVector<MachineOperand*, 8>::const_iterator I = Uses0.begin(), E = Uses0.end(); I != E; ++I)
									(*I)->substVirtReg(DoubleReg, Epiphany::sub_even, *TRI);

								for (SmallVector<MachineOperand*, 8>::const_iterator I = Uses1.begin(), E = Uses1.end(); I != E; ++I)
									(*I)->substVirtReg(DoubleReg, Epiphany::sub_odd, *TRI);

							} else {//store

								//build our dpr
								BuildMI(*MBB, InsertPos, dl, TII->get(Epiphany::REG_SEQUENCE), DoubleReg)
									.addReg(Op0->getOperand(0).getReg())
									.addImm(Epiphany::sub_even)
									.addReg(Op1->getOperand(0).getReg())
									.addImm(Epiphany::sub_odd);


								MachineInstrBuilder MIB = BuildMI(*MBB, InsertPos, dl, MCID)
									.addReg(DoubleReg, RegState::Kill)
									.addReg(BaseReg);
								MIB.addImm(Offset);
								concatenateMemOperands(MIB, Op0, Op1);
								DEBUG(dbgs() << "Formed " << *MIB << "\n");
								++NumSTRDFormed;

							}
							MBB->erase(Op0);
							MBB->erase(Op1);

					} else {
						for (unsigned i = 0; i != NumMove; ++i) {
							MachineInstr *Op = Ops.back();
							Ops.pop_back();
							MBB->splice(InsertPos, MBB, Op);
						}
					}

					NumLdStMoved += NumMove;
					RetVal = true;
				}
			}
		}

		return RetVal;
}

bool
EpiphanyPreAllocLoadStoreOpt::RescheduleLoadStoreInstrs(MachineBasicBlock *MBB) {
  bool RetVal = false;

  DenseMap<MachineInstr*, unsigned> MI2LocMap;
  DenseMap<unsigned, SmallVector<MachineInstr*, 4> > Base2LdsMap;
  DenseMap<unsigned, SmallVector<MachineInstr*, 4> > Base2StsMap;
  SmallVector<unsigned, 4> LdBases;
  SmallVector<unsigned, 4> StBases;

  unsigned Loc = 0;
  MachineBasicBlock::iterator MBBI = MBB->begin();
  MachineBasicBlock::iterator E = MBB->end();
  while (MBBI != E) {
    for (; MBBI != E; ++MBBI) {
      MachineInstr *MI = MBBI;
      if (MI->isCall() || MI->isTerminator()) {
        // Stop at barriers.
        ++MBBI;
        break;
      }

      if (!MI->isDebugValue())
        MI2LocMap[MI] = ++Loc;

      if (!isMemoryOp(MI))
        continue;

      int Opc = MI->getOpcode();
      bool isLd = (Opc == Epiphany::LS32_LDR || Opc == Epiphany::LSFP32_LDR);
      unsigned Base = MI->getOperand(1).getReg();
      int Offset = getMemoryOpOffset(MI);

      bool StopHere = false;
      if (isLd) {
        DenseMap<unsigned, SmallVector<MachineInstr*, 4> >::iterator BI =
          Base2LdsMap.find(Base);
        if (BI != Base2LdsMap.end()) {
          for (unsigned i = 0, e = BI->second.size(); i != e; ++i) {
            if (Offset == getMemoryOpOffset(BI->second[i])) {
              StopHere = true;
              break;
            }
          }
          if (!StopHere)
            BI->second.push_back(MI);
        } else {
          SmallVector<MachineInstr*, 4> MIs;
          MIs.push_back(MI);
          Base2LdsMap[Base] = MIs;
          LdBases.push_back(Base);
        }
      } else {
        DenseMap<unsigned, SmallVector<MachineInstr*, 4> >::iterator BI =
          Base2StsMap.find(Base);
        if (BI != Base2StsMap.end()) {
          for (unsigned i = 0, e = BI->second.size(); i != e; ++i) {
            if (Offset == getMemoryOpOffset(BI->second[i])) {
              StopHere = true;
              break;
            }
          }
          if (!StopHere)
            BI->second.push_back(MI);
        } else {
          SmallVector<MachineInstr*, 4> MIs;
          MIs.push_back(MI);
          Base2StsMap[Base] = MIs;
          StBases.push_back(Base);
        }
      }

      if (StopHere) {
        // Found a duplicate (a base+offset combination that's seen earlier).
        // Backtrack.
        --Loc;
        break;
      }
    }

    // Re-schedule loads.
    for (unsigned i = 0, e = LdBases.size(); i != e; ++i) {
      unsigned Base = LdBases[i];
      SmallVector<MachineInstr*, 4> &Lds = Base2LdsMap[Base];
      if (Lds.size() > 1)
        RetVal |= RescheduleOps(MBB, Lds, Base, true, MI2LocMap);
    }

    // Re-schedule stores.
    for (unsigned i = 0, e = StBases.size(); i != e; ++i) {
      unsigned Base = StBases[i];
      SmallVector<MachineInstr*, 4> &Sts = Base2StsMap[Base];
      if (Sts.size() > 1)
        RetVal |= RescheduleOps(MBB, Sts, Base, false, MI2LocMap);
    }

    if (MBBI != E) {
      Base2LdsMap.clear();
      Base2StsMap.clear();
      LdBases.clear();
      StBases.clear();
    }
  }

  return RetVal;
}


/// createEpiphanyLoadStoreOptimizationPass - returns an instance of the load / store
/// optimization pass.
FunctionPass *llvm::createEpiphanyLSOptPass() {
    return new EpiphanyPreAllocLoadStoreOpt();
}
