//===-- EpiphanySelectionDAGInfo.h - Epiphany SelectionDAG Info ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the Epiphany subclass for TargetSelectionDAGInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANYSELECTIONDAGINFO_H
#define LLVM_EPIPHANYSELECTIONDAGINFO_H

#include "llvm/Target/TargetSelectionDAGInfo.h"

namespace llvm {

class EpiphanyTargetMachine;

class EpiphanySelectionDAGInfo : public TargetSelectionDAGInfo {
  const EpiphanySubtarget *Subtarget;
public:
  explicit EpiphanySelectionDAGInfo(const EpiphanyTargetMachine &TM);
  ~EpiphanySelectionDAGInfo();
};

}

#endif
