//==- EpiphanyMCExpr.h - Epiphany specific MC expression classes --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes Epiphany-specific MCExprs, used for modifiers like
// ":lo12:" or ":gottprel_g1:".
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANYMCEXPR_H
#define LLVM_EPIPHANYMCEXPR_H

#include "llvm/MC/MCExpr.h"

namespace llvm {

class EpiphanyMCExpr : public MCTargetExpr {
public:
  enum VariantKind {
    VK_EPIPHANY_None,
    VK_EPIPHANY_LO16,     // :lo12:
	VK_EPIPHANY_HI16,     // :lo12:
  };

private:
  const VariantKind Kind;
  const MCExpr *Expr;

  explicit EpiphanyMCExpr(VariantKind _Kind, const MCExpr *_Expr)
    : Kind(_Kind), Expr(_Expr) {}

public:
  /// @name Construction
  /// @{

  static const EpiphanyMCExpr *Create(VariantKind Kind, const MCExpr *Expr,
                                     MCContext &Ctx);

  static const EpiphanyMCExpr *CreateLo16(const MCExpr *Expr, MCContext &Ctx) {
    return Create(VK_EPIPHANY_LO16, Expr, Ctx);
  }
  static const EpiphanyMCExpr *CreateHi16(const MCExpr *Expr, MCContext &Ctx) {
    return Create(VK_EPIPHANY_HI16, Expr, Ctx);
  }

  /// @}
  /// @name Accessors
  /// @{

  /// getOpcode - Get the kind of this expression.
  VariantKind getKind() const { return Kind; }

  /// getSubExpr - Get the child of this expression.
  const MCExpr *getSubExpr() const { return Expr; }

  /// @}

  void PrintImpl(raw_ostream &OS) const;
  bool EvaluateAsRelocatableImpl(MCValue &Res,
                                 const MCAsmLayout *Layout) const;
  void AddValueSymbols(MCAssembler *) const;
  const MCSection *FindAssociatedSection() const {
    return getSubExpr()->FindAssociatedSection();
  }

  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const;

  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }

  static bool classof(const EpiphanyMCExpr *) { return true; }

};
} // end namespace llvm

#endif
