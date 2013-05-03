//===- EpiphanyInstrInfo.cpp - Epiphany Instruction Information -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "Epiphany.h"
#include "EpiphanyInstrInfo.h"
#include "EpiphanyMachineFunctionInfo.h"
#include "EpiphanyTargetMachine.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "Utils/EpiphanyBaseInfo.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#include <algorithm>

#define GET_INSTRINFO_CTOR
#include "EpiphanyGenInstrInfo.inc"

using namespace llvm;

EpiphanyInstrInfo::EpiphanyInstrInfo(const EpiphanySubtarget &STI)
  : EpiphanyGenInstrInfo(Epiphany::ADJCALLSTACKDOWN, Epiphany::ADJCALLSTACKUP),
    RI(*this, STI), Subtarget(STI) {}

void EpiphanyInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator I, DebugLoc DL,
                                   unsigned DestReg, unsigned SrcReg,
                                   bool KillSrc) const {

  bool GPRDest = Epiphany::GPR32RegClass.contains(DestReg);
  bool GPRSrc  = Epiphany::GPR32RegClass.contains(SrcReg);

  if (GPRDest && GPRSrc) {
    BuildMI(MBB, I, DL, get(Epiphany::MOVww), DestReg)
		.addReg(SrcReg, getKillRegState(KillSrc));
    return;
  } else if(!GPRDest && !GPRSrc) {
	      BuildMI(MBB, I, DL, get(Epiphany::MOVss), DestReg)
		.addReg(SrcReg, getKillRegState(KillSrc));
    return;
  } 
  
    llvm_unreachable("Unknown register class in copyPhysReg");

}

MachineInstr *
EpiphanyInstrInfo::emitFrameIndexDebugValue(MachineFunction &MF, int FrameIx,
                                           uint64_t Offset, const MDNode *MDPtr,
                                           DebugLoc DL) const {
  MachineInstrBuilder MIB = BuildMI(MF, DL, get(Epiphany::DBG_VALUE))
    .addFrameIndex(FrameIx).addImm(0)
    .addImm(Offset)
    .addMetadata(MDPtr);
  return &*MIB;
}


bool
EpiphanyInstrInfo::analyzeCompare(const MachineInstr *MI, unsigned &SrcReg, unsigned &SrcReg2,
               int &CmpMask, int &CmpValue) const {
  switch (MI->getOpcode()) {
  default: break;
  case Epiphany::SUBri_cmp:
    SrcReg = MI->getOperand(0).getReg();
    SrcReg2 = 0;
    CmpMask = ~0;
    CmpValue = MI->getOperand(1).getImm();
    return true;
  case Epiphany::CMPrr:
    SrcReg = MI->getOperand(0).getReg();
    SrcReg2 = MI->getOperand(1).getReg();
    CmpMask = ~0;
    CmpValue = 0;
    return true;
  }
  return false;
}

/// OptimizeCompareInstr - Convert the instruction supplying the argument to the
/// comparison into one that sets the zero bit in the flags register. Convert
/// the SUBrr(r1,r2)|Subri(r1,CmpValue) instruction into one that sets the flags
/// register and remove the CMPrr(r1,r2)|CMPrr(r2,r1)|CMPri(r1,CmpValue)
/// instruction.
bool
EpiphanyInstrInfo::optimizeCompareInstr(MachineInstr *CmpInstr, unsigned SrcReg, unsigned SrcReg2, int CmpMask, int CmpValue, const MachineRegisterInfo *MRI) const {

  MachineRegisterInfo::def_iterator DI = MRI->def_begin(SrcReg);
  if (llvm::next(DI) != MRI->def_end())
    // Only support one definition.
    return false;

  MachineInstr *MI = &*DI;

  // Get ready to iterate backward from CmpInstr.
  MachineBasicBlock::iterator I = CmpInstr, E = MI, B = CmpInstr->getParent()->begin();

  // Early exit if CmpInstr is at the beginning of the BB.
  if (I == B) return false;

  // There are two possible candidates which can be changed to set CPSR:
  // One is MI, the other is a SUB instruction.
  // For CMPrr(r1,r2), we are looking for SUB(r1,r2) or SUB(r2,r1).
  // For CMPri(r1, CmpValue), we are looking for SUBri(r1, CmpValue).
  MachineInstr *Sub = NULL;
  unsigned SrcReg2X = 0;
  if (CmpInstr->getOpcode() == Epiphany::CMPrr) {
    SrcReg2X = CmpInstr->getOperand(1).getReg();
    // MI is not a candidate for CMPrr.
    MI = NULL;
  } else if (MI->getParent() != CmpInstr->getParent() || CmpValue != 0) {
    // Conservatively refuse to convert an instruction which isn't in the same
    // BB as the comparison.
    // For CMPri, we need to check Sub, thus we can't return here.
	  if(CmpInstr->getOpcode() == Epiphany::SUBri_cmp)
      MI = NULL;
    else
      return false;
  }

  // Check that CPSR isn't set between the comparison instruction and the one we
  // want to change. At the same time, search for Sub.
  --I;
  for (; I != E; --I) {
    const MachineInstr &Instr = *I;

    for (unsigned IO = 0, EO = Instr.getNumOperands(); IO != EO; ++IO) {
      const MachineOperand &MO = Instr.getOperand(IO);
      if (MO.isRegMask() && MO.clobbersPhysReg(Epiphany::NZCV))
        return false;
      if (!MO.isReg()) continue;

      // This instruction modifies or uses CPSR after the one we want to
      // change. We can't do this transformation.
      if (MO.getReg() == Epiphany::NZCV)
        return false;
    }

    // Check whether the current instruction is SUB(r1, r2) or SUB(r2, r1).
    if (SrcReg2X != 0 && Instr.getOpcode() == Epiphany::SUBrr &&
        ((Instr.getOperand(1).getReg() == SrcReg &&
          Instr.getOperand(2).getReg() == SrcReg2X) ||
         (Instr.getOperand(1).getReg() == SrcReg2X &&
          Instr.getOperand(2).getReg() == SrcReg))) {
      Sub = &*I;
      break;
    }

    // Check whether the current instruction is SUBri(r1, CmpValue).
    if ((CmpInstr->getOpcode() == Epiphany::SUBri_cmp) &&
        Instr.getOpcode() == Epiphany::SUBri && CmpValue != 0 &&
        Instr.getOperand(1).getReg() == SrcReg &&
        Instr.getOperand(2).getImm() == CmpValue) {
      Sub = &*I;
      break;
    }

    if (I == B)
      // The 'and' is below the comparison instruction.
      return false;
  }

  // Return false if no candidates exist.
  if (!MI && !Sub)
    return false;

  // The single candidate is called MI.
  if (!MI) MI = Sub;

    switch (MI->getOpcode()) {
  default: break;
  case Epiphany::ADDrr:
  case Epiphany::ADDri:
  case Epiphany::SUBrr:
  case Epiphany::SUBri:
  case Epiphany::ANDrr:
  case Epiphany::ORRrr:
  case Epiphany::EORrr: {
    // Scan forward for the use of CPSR
    // When checking against MI: if it's a conditional code requires
    // checking of V bit, then this is not safe to do. If we can't find the
    // CPSR use (i.e. used in another block), then it's not safe to perform
    // the optimization.
    // When checking against Sub, we handle the condition codes GE, LT, GT, LE.
    SmallVector<MachineOperand*, 4> OperandsToUpdate;
    bool isSafe = false;
    I = CmpInstr;
    E = CmpInstr->getParent()->end();
	while (!isSafe && ++I != E) {
		const MachineInstr &Instr = *I;
		for (unsigned IO = 0, EO = Instr.getNumOperands(); !isSafe && IO != EO; ++IO) {
			const MachineOperand &MO = Instr.getOperand(IO);
			if (MO.isRegMask() && MO.clobbersPhysReg(Epiphany::NZCV)) {
				isSafe = true;
				break;
			}
			if (!MO.isReg() || MO.getReg() != Epiphany::NZCV)
				continue;
			if (MO.isDef()) {
				isSafe = true;
				break;
			}
			
			EpiphanyCC::CondCodes CC;
			switch(Instr.getOpcode()){
			case Epiphany::MOVCCrr:
			case Epiphany::MOVCCss:
				CC = (EpiphanyCC::CondCodes)Instr.getOperand(3).getImm();
				break;
			case Epiphany::Bcc:
				CC = (EpiphanyCC::CondCodes)Instr.getOperand(0).getImm();
				break;
			}
			 
			if (Sub)
				switch (CC) {
				default:
					return false;
				case EpiphanyCC::GTE:
				case EpiphanyCC::LT:
				case EpiphanyCC::GT:
				case EpiphanyCC::LTE:
					// If we have SUB(r1, r2) and CMP(r2, r1), the condition code based
					// on CMP needs to be updated to be based on SUB.
					// Push the condition code operands to OperandsToUpdate.
					// If it is safe to remove CmpInstr, the condition code of these
					// operands will be modified.
					if (SrcReg2 != 0 && Sub->getOperand(1).getReg() == SrcReg2 &&
						Sub->getOperand(2).getReg() == SrcReg)
						OperandsToUpdate.push_back(&((*I).getOperand(IO-1)));
					break;
			}
			else
				switch (CC) {
				default:
					isSafe = true;
					break;
					//case EpiphanyCC::VS:
					//case EpiphanyCC::VC:
				case EpiphanyCC::GTE:
				case EpiphanyCC::LT:
				case EpiphanyCC::GT:
				case EpiphanyCC::LTE:
					return false;
			}
		}
	}

    // If the candidate is Sub, we may exit the loop at end of the basic block.
    // In that case, it is still safe to remove CmpInstr.
    if (!isSafe && !Sub)
      return false;

    // Toggle the optional operand to CPSR.
    //MI->getOperand(5).setReg(Epiphany::NZCV);
    //MI->getOperand(5).setIsDef(true);
    CmpInstr->eraseFromParent();

    // Modify the condition code of operands in OperandsToUpdate.
    // Since we have SUB(r1, r2) and CMP(r2, r1), the condition code needs to
    // be changed from r2 > r1 to r1 < r2, from r2 < r1 to r1 > r2, etc.
    for (unsigned i = 0; i < OperandsToUpdate.size(); i++) {
      EpiphanyCC::CondCodes CC = (EpiphanyCC::CondCodes)OperandsToUpdate[i]->getImm();
      EpiphanyCC::CondCodes NewCC;
      switch(CC) {
      default: break;
      case EpiphanyCC::GTE: NewCC = EpiphanyCC::LTE; break;
      case EpiphanyCC::LT: NewCC = EpiphanyCC::GT; break;
      case EpiphanyCC::GT: NewCC = EpiphanyCC::LT; break;
      case EpiphanyCC::LTE: NewCC = EpiphanyCC::GT; break;
      }
      OperandsToUpdate[i]->setImm(NewCC);
    }
    return true;
  }
  }

  return false;
}

/// Does the Opcode represent a conditional branch that we can remove and re-add
/// at the end of a basic block?
static bool isCondBranch(unsigned Opc) {
  return Opc == Epiphany::Bcc;
}

/// Takes apart a given conditional branch MachineInstr (see isCondBranch),
/// setting TBB to the destination basic block and populating the Cond vector
/// with data necessary to recreate the conditional branch at a later
/// date. First element will be the opcode, and subsequent ones define the
/// conditions being branched on in an instruction-specific manner.
static void classifyCondBranch(MachineInstr *I, MachineBasicBlock *&TBB,
                               SmallVectorImpl<MachineOperand> &Cond) {
  switch(I->getOpcode()) {
  case Epiphany::Bcc:
    // These instructions just have one predicate operand in position 0 (either
    // a condition code or a register being compared).
    Cond.push_back(MachineOperand::CreateImm(I->getOpcode()));
    Cond.push_back(I->getOperand(0));
    TBB = I->getOperand(1).getMBB();
    return;
  default:
    llvm_unreachable("Unknown conditional branch to classify");
  }
}


bool
EpiphanyInstrInfo::AnalyzeBranch(MachineBasicBlock &MBB,MachineBasicBlock *&TBB,
                                MachineBasicBlock *&FBB,
                                SmallVectorImpl<MachineOperand> &Cond,
                                bool AllowModify) const {
  // If the block has no terminators, it just falls into the block after it.
  MachineBasicBlock::iterator I = MBB.end();
  if (I == MBB.begin())
    return false;
  --I;
  while (I->isDebugValue()) {
    if (I == MBB.begin())
      return false;
    --I;
  }
  if (!isUnpredicatedTerminator(I))
    return false;

  // Get the last instruction in the block.
  MachineInstr *LastInst = I;

  // If there is only one terminator instruction, process it.
  unsigned LastOpc = LastInst->getOpcode();
  if (I == MBB.begin() || !isUnpredicatedTerminator(--I)) {
    if (LastOpc == Epiphany::Bimm) {
      TBB = LastInst->getOperand(0).getMBB();
      return false;
    }
    if (isCondBranch(LastOpc)) {
      classifyCondBranch(LastInst, TBB, Cond);
      return false;
    }
    return true;  // Can't handle indirect branch.
  }

  // Get the instruction before it if it is a terminator.
  MachineInstr *SecondLastInst = I;
  unsigned SecondLastOpc = SecondLastInst->getOpcode();

  // If AllowModify is true and the block ends with two or more unconditional
  // branches, delete all but the first unconditional branch.
  if (AllowModify && LastOpc == Epiphany::Bimm) {
    while (SecondLastOpc == Epiphany::Bimm) {
      LastInst->eraseFromParent();
      LastInst = SecondLastInst;
      LastOpc = LastInst->getOpcode();
      if (I == MBB.begin() || !isUnpredicatedTerminator(--I)) {
        // Return now the only terminator is an unconditional branch.
        TBB = LastInst->getOperand(0).getMBB();
        return false;
      } else {
        SecondLastInst = I;
        SecondLastOpc = SecondLastInst->getOpcode();
      }
    }
  }

  // If there are three terminators, we don't know what sort of block this is.
  if (SecondLastInst && I != MBB.begin() && isUnpredicatedTerminator(--I))
    return true;

  // If the block ends with a B and a Bcc, handle it.
  if (LastOpc == Epiphany::Bimm) {
    if (SecondLastOpc == Epiphany::Bcc) {
      TBB =  SecondLastInst->getOperand(1).getMBB();
      Cond.push_back(MachineOperand::CreateImm(Epiphany::Bcc));
      Cond.push_back(SecondLastInst->getOperand(0));
      FBB = LastInst->getOperand(0).getMBB();
      return false;
    } else if (isCondBranch(SecondLastOpc)) {
      classifyCondBranch(SecondLastInst, TBB, Cond);
      FBB = LastInst->getOperand(0).getMBB();
      return false;
    }
  }

  // If the block ends with two unconditional branches, handle it.  The second
  // one is not executed, so remove it.
  if (SecondLastOpc == Epiphany::Bimm && LastOpc == Epiphany::Bimm) {
    TBB = SecondLastInst->getOperand(0).getMBB();
    I = LastInst;
    if (AllowModify)
      I->eraseFromParent();
    return false;
  }

  // Otherwise, can't handle this.
  return true;
}

unsigned
EpiphanyInstrInfo::InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                               MachineBasicBlock *FBB,
                               const SmallVectorImpl<MachineOperand> &Cond,
                               DebugLoc DL) const {
  if (FBB == 0 && Cond.empty()) {
    BuildMI(&MBB, DL, get(Epiphany::Bimm)).addMBB(TBB);
    return 1;
  } else if (FBB == 0) {
    MachineInstrBuilder MIB = BuildMI(&MBB, DL, get(Cond[0].getImm()));
    for (int i = 1, e = Cond.size(); i != e; ++i)
      MIB.addOperand(Cond[i]);
    MIB.addMBB(TBB);
    return 1;
  }

  MachineInstrBuilder MIB = BuildMI(&MBB, DL, get(Cond[0].getImm()));
  for (int i = 1, e = Cond.size(); i != e; ++i)
    MIB.addOperand(Cond[i]);
  MIB.addMBB(TBB);

  BuildMI(&MBB, DL, get(Epiphany::Bimm)).addMBB(FBB);
  return 2;
}

unsigned EpiphanyInstrInfo::RemoveBranch(MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator I = MBB.end();
  if (I == MBB.begin()) return 0;
  --I;
  while (I->isDebugValue()) {
    if (I == MBB.begin())
      return 0;
    --I;
  }
  if (I->getOpcode() != Epiphany::Bimm && !isCondBranch(I->getOpcode()))
    return 0;

  // Remove the branch.
  I->eraseFromParent();

  I = MBB.end();

  if (I == MBB.begin()) return 1;
  --I;
  if (!isCondBranch(I->getOpcode()))
    return 1;

  // Remove the branch.
  I->eraseFromParent();
  return 2;
}

bool
EpiphanyInstrInfo::expandPostRAPseudo(MachineBasicBlock::iterator MBBI) const {
  return false;
}

void
EpiphanyInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator MBBI,
                                      unsigned SrcReg, bool isKill,
                                      int FrameIdx,
                                      const TargetRegisterClass *RC,
                                      const TargetRegisterInfo *TRI) const {
  DebugLoc DL = MBB.findDebugLoc(MBBI);
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = *MF.getFrameInfo();
  unsigned Align = MFI.getObjectAlignment(FrameIdx);

  MachineMemOperand *MMO
    = MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(FrameIdx),
                              MachineMemOperand::MOStore,
                              MFI.getObjectSize(FrameIdx),
                              Align);

  unsigned StoreOp = 0;
  if (RC->hasType(MVT::i64) || RC->hasType(MVT::i32)) {
    switch(RC->getSize()) {
    case 4: StoreOp = Epiphany::LS32_STR; break;
    //case 8: StoreOp = Epiphany::LS64_STR; break;
    default:
      llvm_unreachable("Unknown size for regclass");
    }
  } else {
    assert((RC->hasType(MVT::f32) || RC->hasType(MVT::f64))
           && "Expected integer or floating type for store");
    switch (RC->getSize()) {
    case 4: StoreOp = Epiphany::LSFP32_STR; break;
    //case 8: StoreOp = Epiphany::LSFP64_STR; break;
    default:
      llvm_unreachable("Unknown size for regclass");
    }
  }

  MachineInstrBuilder NewMI = BuildMI(MBB, MBBI, DL, get(StoreOp));
  NewMI.addReg(SrcReg, getKillRegState(isKill))
    .addFrameIndex(FrameIdx)
    .addImm(0)
    .addMemOperand(MMO);

}

void
EpiphanyInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MBBI,
                                       unsigned DestReg, int FrameIdx,
                                       const TargetRegisterClass *RC,
                                       const TargetRegisterInfo *TRI) const {
  DebugLoc DL = MBB.findDebugLoc(MBBI);
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = *MF.getFrameInfo();
  unsigned Align = MFI.getObjectAlignment(FrameIdx);

  MachineMemOperand *MMO
    = MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(FrameIdx),
                              MachineMemOperand::MOLoad,
                              MFI.getObjectSize(FrameIdx),
                              Align);

  unsigned LoadOp = 0;
  if (RC->hasType(MVT::i64) || RC->hasType(MVT::i32)) {
    switch(RC->getSize()) {
    case 4: LoadOp = Epiphany::LS32_LDR; break;
    //case 8: LoadOp = Epiphany::LS64_LDR; break;
    default:
      llvm_unreachable("Unknown size for regclass");
    }
  } else {
    assert((RC->hasType(MVT::f32) || RC->hasType(MVT::f64))
           && "Expected integer or floating type for store");
    switch (RC->getSize()) {
    case 4: LoadOp = Epiphany::LSFP32_LDR; break;
    //case 8: LoadOp = Epiphany::LSFP64_LDR; break;
    default:
      llvm_unreachable("Unknown size for regclass");
    }
  }

  MachineInstrBuilder NewMI = BuildMI(MBB, MBBI, DL, get(LoadOp), DestReg);
  NewMI.addFrameIndex(FrameIdx)
       .addImm(0)
       .addMemOperand(MMO);
}

unsigned EpiphanyInstrInfo::estimateRSStackLimit(MachineFunction &MF) const {
  unsigned Limit = (1 << 16) - 1;
  for (MachineFunction::iterator BB = MF.begin(),E = MF.end(); BB != E; ++BB) {
    for (MachineBasicBlock::iterator I = BB->begin(), E = BB->end();
         I != E; ++I) {
      for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i) {
        if (!I->getOperand(i).isFI()) continue;

        // When using ADDwwi_lsl0_s to get the address of a stack object, 0x3FF
        // is the largest offset guaranteed to fit in the immediate offset.
        if (I->getOpcode() == Epiphany::ADDri) {
          Limit = std::min(Limit, 0x3FFu);
          break;
        }

        int AccessScale, MinOffset, MaxOffset;
        getAddressConstraints(*I, AccessScale, MinOffset, MaxOffset);
        Limit = std::min(Limit, static_cast<unsigned>(MaxOffset));

        break; // At most one FI per instruction
      }
    }
  }

  return Limit;
}
void EpiphanyInstrInfo::getAddressConstraints(const MachineInstr &MI,
                                             int &AccessScale, int &MinOffset,
                                             int &MaxOffset) const {
  switch (MI.getOpcode()) {
  default: llvm_unreachable("Unkown load/store kind");
  case TargetOpcode::DBG_VALUE:
    AccessScale = 1;
    MinOffset = INT_MIN;
    MaxOffset = INT_MAX;
    return;
  case Epiphany::LS8_LDR: case Epiphany::LS8_STR:
    AccessScale = 1;
    MinOffset = 0;
    MaxOffset = 0x7FF;
    return;
  case Epiphany::LS16_LDR: case Epiphany::LS16_STR:
    AccessScale = 2;
    MinOffset = 0;
    MaxOffset = 0x7FF * AccessScale;
    return;
  case Epiphany::LS32_LDR:  case Epiphany::LS32_STR:
  case Epiphany::LSFP32_LDR: case Epiphany::LSFP32_STR:
    AccessScale = 4;
    MinOffset = 0;
    MaxOffset = 0x7FF * AccessScale;
    return;
  //case Epiphany::LSFP64_LDR: case Epiphany::LSFP64_STR:
  //  AccessScale = 8;
  //  MinOffset = 0;
  //  MaxOffset = 0x7FF * AccessScale;
  //  return;
  }
}

unsigned EpiphanyInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  const MCInstrDesc &MCID = MI.getDesc();
  const MachineBasicBlock &MBB = *MI.getParent();
  const MachineFunction &MF = *MBB.getParent();
  const MCAsmInfo &MAI = *MF.getTarget().getMCAsmInfo();

  if (MCID.getSize())
    return MCID.getSize();

  if (MI.getOpcode() == Epiphany::INLINEASM)
    return getInlineAsmLength(MI.getOperand(0).getSymbolName(), MAI);

  if (MI.isLabel())
    return 0;

  switch (MI.getOpcode()) {
  case TargetOpcode::BUNDLE:
    return getInstBundleLength(MI);
  case TargetOpcode::IMPLICIT_DEF:
  case TargetOpcode::KILL:
  case TargetOpcode::PROLOG_LABEL:
  case TargetOpcode::EH_LABEL:
  case TargetOpcode::DBG_VALUE:
    return 0;
  default:
    llvm_unreachable("Unknown instruction class");
  }
}

unsigned EpiphanyInstrInfo::getInstBundleLength(const MachineInstr &MI) const {
  unsigned Size = 0;
  MachineBasicBlock::const_instr_iterator I = MI;
  MachineBasicBlock::const_instr_iterator E = MI.getParent()->instr_end();
  while (++I != E && I->isInsideBundle()) {
    assert(!I->isBundle() && "No nested bundle!");
    Size += getInstSizeInBytes(*I);
  }
  return Size;
}

bool llvm::rewriteA64FrameIndex(MachineInstr &MI, unsigned FrameRegIdx,
                                unsigned FrameReg, int &Offset,
                                const EpiphanyInstrInfo &TII) {
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = *MF.getFrameInfo();

  MFI.getObjectOffset(FrameRegIdx);
  llvm_unreachable("Unimplemented rewriteFrameIndex");
}

void llvm::EPIPHemitRegUpdate(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI,
                         DebugLoc dl, const TargetInstrInfo &TII,
                         unsigned DstReg, unsigned SrcReg, unsigned ScratchReg,
                         int64_t NumBytes, MachineInstr::MIFlag MIFlags) {
  if (NumBytes == 0 && DstReg == SrcReg)
    return;
  else if (abs(NumBytes) & ~0x3FF) { // 11bit signed = 10b unsigned
    // Generically, we have to materialize the offset into a temporary register
    // and subtract it. There are a couple of ways this could be done, for now
    // we'll use a movz/movk or movn/movk sequence.
    uint64_t Bits = static_cast<uint64_t>(abs(NumBytes));
	BuildMI(MBB, MBBI, dl, TII.get(Epiphany::MOVri), ScratchReg)
      .addImm(0xffff & Bits).setMIFlags(MIFlags);

    Bits >>= 16;
    if (Bits & 0xffff) {
      BuildMI(MBB, MBBI, dl, TII.get(Epiphany::MOVTri), ScratchReg)
        .addImm(0xffff & Bits).setMIFlags(MIFlags);
    }

	//HACK since we have only 32b at most this needs only a mov + movt
    // ADD DST, SRC, xTMP (, lsl #0)
    unsigned AddOp = NumBytes > 0 ? Epiphany::ADDrr : Epiphany::SUBrr;
    BuildMI(MBB, MBBI, dl, TII.get(AddOp), DstReg)
      .addReg(SrcReg, RegState::Kill)
      .addReg(ScratchReg, RegState::Kill)
      .addImm(0)
      .setMIFlag(MIFlags);
    return;
  }

  // Now we know that the adjustment can be done in at most two add/sub
  // (immediate) instructions, which is always more efficient than a
  // literal-pool load, or even a hypothetical movz/movk/add sequence

  // Decide whether we're doing addition or subtraction
  unsigned LowOp, HighOp;
  if (NumBytes >= 0) {
    LowOp = Epiphany::ADDri;
    //HighOp = Epiphany::ADDwwi_lsl12_s;
  } else {
    LowOp = Epiphany::SUBri;
    //HighOp = Epiphany::SUBwwi_lsl12_s;
    NumBytes = abs(NumBytes);
  }

  // If we're here, at the very least a move needs to be produced, which just
  // happens to be materializable by an ADD.
  if ((NumBytes & 0x3FF) || NumBytes == 0) {
    BuildMI(MBB, MBBI, dl, TII.get(LowOp), DstReg)
      .addReg(SrcReg, RegState::Kill)
      .addImm(NumBytes & 0x3FF)
      .setMIFlag(MIFlags);

    // Next update should use the register we've just defined.
    SrcReg = DstReg;
  }

  if (NumBytes & 0xfff000) {
	  llvm_unreachable("oh fuck.... hi bits reg update");
    BuildMI(MBB, MBBI, dl, TII.get(HighOp), DstReg)
      .addReg(SrcReg, RegState::Kill)
      .addImm(NumBytes >> 12)
      .setMIFlag(MIFlags);
  }
}

void llvm::EPIPHemitSPUpdate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                        DebugLoc dl, const TargetInstrInfo &TII,
                        unsigned ScratchReg, int64_t NumBytes,
                        MachineInstr::MIFlag MIFlags) {
  EPIPHemitRegUpdate(MBB, MI, dl, TII, Epiphany::SP, Epiphany::SP, Epiphany::R63,
                NumBytes, MIFlags);
}
