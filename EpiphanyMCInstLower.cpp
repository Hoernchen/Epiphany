//===-- EpiphanyMCInstLower.cpp - Convert Epiphany MachineInstr to an MCInst -==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower Epiphany MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyAsmPrinter.h"
#include "EpiphanyTargetMachine.h"
#include "MCTargetDesc/EpiphanyMCExpr.h"
#include "Utils/EpiphanyBaseInfo.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Target/Mangler.h"

using namespace llvm;

MCOperand
EpiphanyAsmPrinter::lowerSymbolOperand(const MachineOperand &MO,
                                      const MCSymbol *Sym) const {
  const MCExpr *Expr = 0;

  Expr = MCSymbolRefExpr::Create(Sym, MCSymbolRefExpr::VK_None, OutContext);

  switch (MO.getTargetFlags()) {
  case EpiphanyII::MO_LO16:
    Expr = EpiphanyMCExpr::CreateLo16(Expr, OutContext);
    break;
  case EpiphanyII::MO_HI16:
    Expr = EpiphanyMCExpr::CreateHi16(Expr, OutContext);
    break;
  case EpiphanyII::MO_NO_FLAG:
    // Expr is already correct
    break;
  default:
    llvm_unreachable("Unexpected MachineOperand flag - lowersymboloperand");
  }

  if (!MO.isJTI() && MO.getOffset())
    Expr = MCBinaryExpr::CreateAdd(Expr,
                                   MCConstantExpr::Create(MO.getOffset(),
                                                          OutContext),
                                   OutContext);

  return MCOperand::CreateExpr(Expr);
}

bool EpiphanyAsmPrinter::lowerOperand(const MachineOperand &MO,
                                     MCOperand &MCOp) const {
  switch (MO.getType()) {
  default: llvm_unreachable("unknown operand type");
  case MachineOperand::MO_Register:
    if (MO.isImplicit())
      return false;
    assert(!MO.getSubReg() && "Subregs should be eliminated!");
    MCOp = MCOperand::CreateReg(MO.getReg());
    break;
  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::CreateImm(MO.getImm());
    break;
  case MachineOperand::MO_FPImmediate: {// a bit hacky, see arm
    APFloat Val = MO.getFPImm()->getValueAPF();
    bool ignored;
    Val.convert(APFloat::IEEEdouble, APFloat::rmTowardZero, &ignored);
    MCOp = MCOperand::CreateFPImm(Val.convertToDouble());
    break;
   }
  case MachineOperand::MO_BlockAddress:
    MCOp = lowerSymbolOperand(MO, GetBlockAddressSymbol(MO.getBlockAddress()));
    break;
  case MachineOperand::MO_ExternalSymbol:
    MCOp = lowerSymbolOperand(MO, GetExternalSymbolSymbol(MO.getSymbolName()));
    break;
  case MachineOperand::MO_GlobalAddress:
    MCOp = lowerSymbolOperand(MO, Mang->getSymbol(MO.getGlobal()));
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MCOp = MCOperand::CreateExpr(MCSymbolRefExpr::Create(
                                   MO.getMBB()->getSymbol(), OutContext));
    break;
  case MachineOperand::MO_JumpTableIndex:
    MCOp = lowerSymbolOperand(MO, GetJTISymbol(MO.getIndex()));
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    MCOp = lowerSymbolOperand(MO, GetCPISymbol(MO.getIndex()));
    break;
  case MachineOperand::MO_RegisterMask:
    // Ignore call clobbers
    return false;

  }

  return true;
}

void llvm::LowerEpiphanyMachineInstrToMCInst(const MachineInstr *MI,
                                            MCInst &OutMI,
                                            EpiphanyAsmPrinter &AP) {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);

    MCOperand MCOp;
    if (AP.lowerOperand(MO, MCOp))
      OutMI.addOperand(MCOp);
  }
}
