//===-- EpiphanyMCTargetDesc.h - Epiphany Target Descriptions -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Epiphany specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANYMCTARGETDESC_H
#define LLVM_EPIPHANYMCTARGETDESC_H

#include "llvm/Support/DataTypes.h"

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class StringRef;
class Target;
class raw_ostream;

extern Target TheEpiphanyTarget;

namespace Epiphany_MC {
  MCSubtargetInfo *createEpiphanyMCSubtargetInfo(StringRef TT, StringRef CPU,
                                                StringRef FS);
}

//MCCodeEmitter *createEpiphanyMCCodeEmitter(const MCInstrInfo &MCII,
//                                          const MCRegisterInfo &MRI,
//                                          const MCSubtargetInfo &STI,
//                                          MCContext &Ctx);

//MCObjectWriter *createEpiphanyELFObjectWriter(raw_ostream &OS,
//                                             uint8_t OSABI);

//MCAsmBackend *createEpiphanyAsmBackend(const Target &T, StringRef TT,
//                                      StringRef CPU);

} // End llvm namespace

// Defines symbolic names for Epiphany registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "EpiphanyGenRegisterInfo.inc"

// Defines symbolic names for the Epiphany instructions.
//
#define GET_INSTRINFO_ENUM
#include "EpiphanyGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "EpiphanyGenSubtargetInfo.inc"

#endif
