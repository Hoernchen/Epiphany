//===-- EpiphanyTargetObjectFile.cpp - Epiphany Object Info -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file deals with any Epiphany specific requirements on object files.
//
//===----------------------------------------------------------------------===//


#include "EpiphanyTargetObjectFile.h"

using namespace llvm;

void
EpiphanyLinuxTargetObjectFile::Initialize(MCContext &Ctx,
                                         const TargetMachine &TM) {
  TargetLoweringObjectFileELF::Initialize(Ctx, TM);
  InitializeELF(TM.Options.UseInitArray);
}
