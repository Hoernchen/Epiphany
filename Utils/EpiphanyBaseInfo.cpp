//===-- EpiphanyBaseInfo.cpp - Epiphany Base encoding information------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides basic encoding and assembly information for Epiphany.
//
//===----------------------------------------------------------------------===//
#include "EpiphanyBaseInfo.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Regex.h"

using namespace llvm;

bool EpiphanyImms::isFPImm(const APFloat &Val, uint32_t &Imm8Bits) {
  const fltSemantics &Sem = Val.getSemantics();
  unsigned FracBits = APFloat::semanticsPrecision(Sem) - 1;

  uint32_t ExpMask;
  switch (FracBits) {
  case 10: // IEEE half-precision
    ExpMask = 0x1f;
    break;
  case 23: // IEEE single-precision
    ExpMask = 0xff;
    break;
  case 52: // IEEE double-precision
    ExpMask = 0x7ff;
    break;
  case 112: // IEEE quad-precision
    // No immediates are valid for double precision.
    return false;
  default:
    llvm_unreachable("Only half, single and double precision supported");
  }

  uint32_t ExpStart = FracBits;
  uint64_t FracMask = (1ULL << FracBits) - 1;

  uint32_t Sign = Val.isNegative();

  uint64_t Bits= Val.bitcastToAPInt().getLimitedValue();
  uint64_t Fraction = Bits & FracMask;
  int32_t Exponent = ((Bits >> ExpStart) & ExpMask);
  Exponent -= ExpMask >> 1;

  // S[d] = imm8<7>:NOT(imm8<6>):Replicate(imm8<6>, 5):imm8<5:0>:Zeros(19)
  // D[d] = imm8<7>:NOT(imm8<6>):Replicate(imm8<6>, 8):imm8<5:0>:Zeros(48)
  // This translates to: only 4 bits of fraction; -3 <= exp <= 4.
  uint64_t A64FracStart = FracBits - 4;
  uint64_t A64FracMask = 0xf;

  // Are there too many fraction bits?
  if (Fraction & ~(A64FracMask << A64FracStart))
    return false;

  if (Exponent < -3 || Exponent > 4)
    return false;

  uint32_t PackedFraction = (Fraction >> A64FracStart) & A64FracMask;
  uint32_t PackedExp = (Exponent + 7) & 0x7;

  Imm8Bits = (Sign << 7) | (PackedExp << 4) | PackedFraction;
  return true;
}
