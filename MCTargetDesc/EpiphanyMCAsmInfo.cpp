//===-- EpiphanyMCAsmInfo.cpp - Epiphany asm properties ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the EpiphanyMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyMCAsmInfo.h"

using namespace llvm;

EpiphanyELFMCAsmInfo::EpiphanyELFMCAsmInfo() {
  PointerSize = 8;

  // ".comm align is in bytes but .align is pow-2."
  AlignmentIsInBytes = false;

  CommentString = "//";
  PrivateGlobalPrefix = ".L";
  Code32Directive = ".code\t32";

  Data16bitsDirective = "\t.hword\t";
  Data32bitsDirective = "\t.word\t";
  Data64bitsDirective = "\t.xword\t";

  UseDataRegionDirectives = true;

  WeakRefDirective = "\t.weak\t";

  HasLEB128 = true;
  SupportsDebugInformation = true;

  // Exceptions handling
  ExceptionsType = ExceptionHandling::DwarfCFI;
}
