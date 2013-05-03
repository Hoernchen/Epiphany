//===-- EpiphanyBaseInfo.h - Top level definitions for Epiphany- --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the Epiphany target useful for the compiler back-end and the MC libraries.
// As such, it deliberately does not include references to LLVM core
// code gen types, passes, etc..
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANY_BASEINFO_H
#define LLVM_EPIPHANY_BASEINFO_H

#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

// // Enums corresponding to Epiphany condition codes
namespace EpiphanyCC {
	// The CondCodes constants map directly to the 4-bit encoding of the
	// condition field for predicated instructions.
	enum CondCodes {
		EQ = 0,
		NE,
		GTU,
		GTEU,
		LTEU,
		LTU,
		GT,
		GTE,
		LT,
		LTE,
		BEQ,
		BNE,
		BLT,
		BLTE,
		AL, // unconditional, always
		L, // link
		Invalid
	};

} // namespace EpiphanyCC

inline static const char *A64CondCodeToString(EpiphanyCC::CondCodes CC) {
	switch (CC) {
	default: llvm_unreachable("Unknown condition code");
	case EpiphanyCC::EQ:  return "eq";
	case EpiphanyCC::NE:  return "ne";
	case EpiphanyCC::GTU:  return "gtu";
	case EpiphanyCC::GTEU:  return "gteu";
	case EpiphanyCC::LTEU:  return "lteu";
	case EpiphanyCC::LTU:  return "ltu";
	case EpiphanyCC::GT:  return "gt";
	case EpiphanyCC::GTE:  return "gte";
	case EpiphanyCC::LT:  return "lt";
	case EpiphanyCC::LTE:  return "lte";
	case EpiphanyCC::BEQ:  return "beq";
	case EpiphanyCC::BNE:  return "bne";
	case EpiphanyCC::BLT:  return "blt";
	case EpiphanyCC::BLTE:  return "blte";
	case EpiphanyCC::AL:  return "al"; // unconditional, always
	case EpiphanyCC::L:  return "l"; // link

	}
}
namespace EpiphanyII {

	enum TOF {
		//===--------------------------------------------------------------===//
		// Epiphany Specific MachineOperand flags.

		MO_NO_FLAG,

		// LO/HI reloc, only for symbols
		MO_LO16,
		MO_HI16
	};
}

class APFloat;

namespace EpiphanyImms {
	bool isFPImm(const APFloat &Val, uint32_t &Imm8Bits);

	inline bool isFPImm(const APFloat &Val) {
		uint32_t Imm8;
		return isFPImm(Val, Imm8);
	}

}

} // end namespace llvm;

#endif
