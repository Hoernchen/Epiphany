//===-- EpiphanySelectionDAGInfo.cpp - Epiphany SelectionDAG Info -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the EpiphanySelectionDAGInfo class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "arm-selectiondag-info"
#include "EpiphanyTargetMachine.h"
#include "llvm/CodeGen/SelectionDAG.h"
using namespace llvm;

EpiphanySelectionDAGInfo::EpiphanySelectionDAGInfo(const EpiphanyTargetMachine &TM)
  : TargetSelectionDAGInfo(TM),
    Subtarget(&TM.getSubtarget<EpiphanySubtarget>()) {
}

EpiphanySelectionDAGInfo::~EpiphanySelectionDAGInfo() {
}
