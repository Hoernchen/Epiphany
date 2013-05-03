//===- EpiphanyRegisterInfo.cpp - Epiphany Register Information -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the TargetRegisterInfo
// class.
//
//===----------------------------------------------------------------------===//


#include "EpiphanyRegisterInfo.h"
#include "EpiphanyFrameLowering.h"
#include "EpiphanyMachineFunctionInfo.h"
#include "EpiphanyTargetMachine.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/VirtRegMap.h" 
#include "llvm/ADT/BitVector.h"

#define GET_REGINFO_TARGET_DESC
#include "EpiphanyGenRegisterInfo.inc"

using namespace llvm;

EpiphanyRegisterInfo::EpiphanyRegisterInfo(const EpiphanyInstrInfo &tii,
                                         const EpiphanySubtarget &sti)
  : EpiphanyGenRegisterInfo(Epiphany::LR), TII(tii) {
}

const uint16_t *
EpiphanyRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_PCS_SaveList;
}

const uint32_t*
EpiphanyRegisterInfo::getCallPreservedMask(CallingConv::ID) const {
  return CSR_PCS_RegMask;
}

const TargetRegisterClass *
EpiphanyRegisterInfo::getCrossCopyRegClass(const TargetRegisterClass *RC) const {
  //if (RC == &Epiphany::FlagClassRegClass)
  //  return &Epiphany::GPR32RegClass;

  return RC;
}



BitVector
EpiphanyRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  const TargetFrameLowering *TFI = MF.getTarget().getFrameLowering();

  Reserved.set(Epiphany::SP);
  Reserved.set(Epiphany::R63);// hack: dst for cmp
  //constants
  Reserved.set(Epiphany::R28);
  Reserved.set(Epiphany::R29);
  Reserved.set(Epiphany::R30);
  Reserved.set(Epiphany::R31);

  Reserved.set(Epiphany::NZCV);

  if (TFI->hasFP(MF)) {
    Reserved.set(Epiphany::R11);
  }

  return Reserved;
}

void
EpiphanyRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MBBI,
                                         int SPAdj,
                                         unsigned FIOperandNum,
                                         RegScavenger *RS) const {
  assert(SPAdj == 0 && "Cannot deal with nonzero SPAdj yet");
  MachineInstr &MI = *MBBI;
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  const EpiphanyFrameLowering *TFI = static_cast<const EpiphanyFrameLowering *>(MF.getTarget().getFrameLowering());

  // In order to work out the base and offset for addressing, the FrameLowering
  // code needs to know (sometimes) whether the instruction is storing/loading a
  // callee-saved register, or whether it's a more generic
  // operation. Fortunately the frame indices are used *only* for that purpose
  // and are contiguous, so we can check here.
  const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();
  int MinCSFI = 0;
  int MaxCSFI = -1;

  if (CSI.size()) {
    MinCSFI = CSI[0].getFrameIdx();
    MaxCSFI = CSI[CSI.size() - 1].getFrameIdx();
  }

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  bool IsCalleeSaveOp = FrameIndex >= MinCSFI && FrameIndex <= MaxCSFI;

  unsigned FrameReg;
  int64_t Offset;
  Offset = TFI->resolveFrameIndexReference(MF, FrameIndex, FrameReg, SPAdj, IsCalleeSaveOp);

  Offset += MI.getOperand(FIOperandNum + 1).getImm();

  // DBG_VALUE instructions have no real restrictions so they can be handled
  // easily.
  if (MI.isDebugValue()) {
    MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, /*isDef=*/ false);
    MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
    return;
  }

  int MinOffset, MaxOffset, OffsetScale;
  if (MI.getOpcode() == Epiphany::ADDri) {
    MinOffset = -(0x3FF);
    MaxOffset = 0x3FF;
    OffsetScale = 1;
  } else {
    // Load/store of a stack object
    TII.getAddressConstraints(MI, OffsetScale, MinOffset, MaxOffset);
  }

  // The frame lowering has told us a base and offset it thinks we should use to
  // access this variable, but it's still up to us to make sure the values are
  // legal for the instruction in question.
  if (Offset % OffsetScale != 0 || Offset < MinOffset || Offset > MaxOffset) {
    unsigned BaseReg = MF.getRegInfo().createVirtualRegister(&Epiphany::GPR32RegClass);
    EPIPHemitRegUpdate(MBB, MBBI, MBBI->getDebugLoc(), TII, BaseReg, FrameReg, BaseReg, Offset);
    FrameReg = BaseReg;
    Offset = 0;
  }

  // Negative offsets are expected if we address from FP, but for
  // now this checks nothing has gone horribly wrong.
  assert(Offset >= 0 && "Unexpected negative offset from SP");

  MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false, false, true);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset / OffsetScale);
}

unsigned
EpiphanyRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getTarget().getFrameLowering();

  if (TFI->hasFP(MF))
    return Epiphany::R11;
  else
    return Epiphany::SP;
}

bool
EpiphanyRegisterInfo::useFPForScavengingIndex(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getTarget().getFrameLowering();
  const EpiphanyFrameLowering *AFI = static_cast<const EpiphanyFrameLowering*>(TFI);
  return AFI->useFPForAddressing(MF);
}
