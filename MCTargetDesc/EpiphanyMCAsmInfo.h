//==-- EpiphanyMCAsmInfo.h - Epiphany asm properties -------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the EpiphanyMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANYTARGETASMINFO_H
#define LLVM_EPIPHANYTARGETASMINFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

  struct EpiphanyELFMCAsmInfo : public MCAsmInfo {
    explicit EpiphanyELFMCAsmInfo();
  };

} // namespace llvm

#endif
