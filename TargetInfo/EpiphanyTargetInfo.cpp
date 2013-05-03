//===-- EpiphanyTargetInfo.cpp - Epiphany Target Implementation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the key registration step for the architecture.
//
//===----------------------------------------------------------------------===//

#include "Epiphany.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target llvm::TheEpiphanyTarget;

extern "C" void LLVMInitializeEpiphanyTargetInfo() {
  RegisterTarget<Triple::epiphany>
    X(TheEpiphanyTarget, "epiphany", "Epiphany");
}
