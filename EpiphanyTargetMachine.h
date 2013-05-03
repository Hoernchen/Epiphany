//=== EpiphanyTargetMachine.h - Define TargetMachine for Epiphany -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Epiphany specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANYTARGETMACHINE_H
#define LLVM_EPIPHANYTARGETMACHINE_H

#include "EpiphanyFrameLowering.h"
#include "EpiphanyISelLowering.h"
#include "EpiphanyInstrInfo.h"
#include "EpiphanySelectionDAGInfo.h"
#include "EpiphanySubtarget.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class EpiphanyTargetMachine : public LLVMTargetMachine {
  EpiphanySubtarget          Subtarget;
  EpiphanyInstrInfo          InstrInfo;
  const DataLayout          DL;
  EpiphanyTargetLowering     TLInfo;
  EpiphanySelectionDAGInfo   TSInfo;
  EpiphanyFrameLowering      FrameLowering;

public:
  EpiphanyTargetMachine(const Target &T, StringRef TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Reloc::Model RM, CodeModel::Model CM,
                       CodeGenOpt::Level OL);

  const EpiphanyInstrInfo *getInstrInfo() const {
    return &InstrInfo;
  }

  const EpiphanyFrameLowering *getFrameLowering() const {
    return &FrameLowering;
  }

  const EpiphanyTargetLowering *getTargetLowering() const {
    return &TLInfo;
  }

  const EpiphanySelectionDAGInfo *getSelectionDAGInfo() const {
    return &TSInfo;
  }

  const EpiphanySubtarget *getSubtargetImpl() const { return &Subtarget; }

  const DataLayout *getDataLayout() const { return &DL; }

  const TargetRegisterInfo *getRegisterInfo() const {
    return &InstrInfo.getRegisterInfo();
  }
  TargetPassConfig *createPassConfig(PassManagerBase &PM);
};

}

#endif
