//==-- EpiphanyInstPrinter.cpp - Convert Epiphany MCInst to assembly syntax --==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an Epiphany MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"
#include "EpiphanyInstPrinter.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "Utils/EpiphanyBaseInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define GET_INSTRUCTION_NAME
#define PRINT_ALIAS_INSTR
#include "EpiphanyGenAsmWriter.inc"

EpiphanyInstPrinter::EpiphanyInstPrinter(const MCAsmInfo &MAI,
                                       const MCInstrInfo &MII,
                                       const MCRegisterInfo &MRI,
                                       const MCSubtargetInfo &STI) :
  MCInstPrinter(MAI, MII, MRI) {
  // Initialize the set of available features.
  setAvailableFeatures(STI.getFeatureBits());
}

void EpiphanyInstPrinter::printAddSubImmOperand(const MCInst *MI, unsigned OpNum, raw_ostream &O) {
		const MCOperand &Imm11Op = MI->getOperand(OpNum);
		int64_t Imm11 = Imm11Op.getImm();
		assert((Imm11 <= 1023 && Imm11 >= -1024) && "Invalid immediate for add/sub imm");

		O << "#" << Imm11;
}

void EpiphanyInstPrinter::printBareImmOperand(const MCInst *MI, unsigned OpNum, raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNum);
  O << MO.getImm();
}


void EpiphanyInstPrinter::printCondCodeOperand(const MCInst *MI, unsigned OpNum, raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNum);

  O << A64CondCodeToString(static_cast<EpiphanyCC::CondCodes>(MO.getImm()));
}

template <unsigned field_width, unsigned scale> void
EpiphanyInstPrinter::printLabelOperand(const MCInst *MI, unsigned OpNum, raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNum);

  if (!MO.isImm()) {
    printOperand(MI, OpNum, O);
    return;
  }

//we do currently not support branches to immediates that are not labels
assert(MO.isImm() && "unknown operand kind in printLabelOperand (imm?)");
}

void EpiphanyInstPrinter::printOffsetUImm11Operand(const MCInst *MI, unsigned OpNum, raw_ostream &O, int MemSize) {
  const MCOperand &MOImm = MI->getOperand(OpNum);
    int32_t Imm = MOImm.getImm();// * MemSize;

    O << "#" << Imm;
}

void EpiphanyInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                      raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    unsigned Reg = Op.getReg();
    O << getRegisterName(Reg);
  } else if (Op.isImm()) {
    O << '#' << Op.getImm();
  } else {
    assert(Op.isExpr() && "unknown operand kind in printOperand");
    // If a symbolic branch target was added as a constant expression then print
    // that address in hex.
    const MCConstantExpr *BranchTarget = dyn_cast<MCConstantExpr>(Op.getExpr());
    int64_t Address;
    if (BranchTarget && BranchTarget->EvaluateAsAbsolute(Address)) {
      O << "0x";
      O.write_hex(Address);
    }
    else {
      // Otherwise, just print the expression.
      O << *Op.getExpr();
    }
  }
}

void EpiphanyInstPrinter::printFPImmOperand(const MCInst *MI, unsigned OpNum,
                                           raw_ostream &o) {
  const MCOperand &MOImm = MI->getOperand(OpNum);
  assert(MOImm.isFPImm() && "not float operand in printFPImmOperand");
  o << '#' << format("%.8f", MOImm.getFPImm());
}

void EpiphanyInstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                                   StringRef Annot) {
if (!printAliasInstr(MI, O))
    printInstruction(MI, O);

  printAnnotation(O, Annot);
}
