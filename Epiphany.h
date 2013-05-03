//==-- Epiphany.h - Top-level interface for Epiphany representation -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// Epiphany back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_EPIPHANY_H
#define LLVM_TARGET_EPIPHANY_H

#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class EpiphanyAsmPrinter;
class FunctionPass;
class EpiphanyTargetMachine;
class MachineInstr;
class MCInst;

FunctionPass *createEpiphanyISelDAG(EpiphanyTargetMachine &TM,
                                   CodeGenOpt::Level OptLevel);

FunctionPass *createEpiphanyCondMovPass(EpiphanyTargetMachine &TM);

FunctionPass *createEpiphanyLSOptPass();

void LowerEpiphanyMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                      EpiphanyAsmPrinter &AP);


}

#endif
