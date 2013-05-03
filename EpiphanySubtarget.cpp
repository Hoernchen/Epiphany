//===-- EpiphanySubtarget.cpp - Epiphany Subtarget Information --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Epiphany specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "EpiphanySubtarget.h"
#include "EpiphanyRegisterInfo.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/SmallVector.h"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "EpiphanyGenSubtargetInfo.inc"

using namespace llvm;

EpiphanySubtarget::EpiphanySubtarget(StringRef TT, StringRef CPU, StringRef FS)
  : EpiphanyGenSubtargetInfo(TT, CPU, FS)
  , TargetTriple(TT) {

  ParseSubtargetFeatures(CPU, FS);
}

bool EpiphanySubtarget::GVIsIndirectSymbol(const GlobalValue *GV,
                                          Reloc::Model RelocM) const {
  if (RelocM == Reloc::Static)
    return false;

  return !GV->hasLocalLinkage() && !GV->hasHiddenVisibility();
}
