//===- EpiphanyInstrFormats.td - Epiphany Instruction Formats --*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This file describes Epiphany instruction formats, down to the level of the
// instruction's overall class.
// ===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// A64 Instruction Format Definitions.
//===----------------------------------------------------------------------===//

class PseudoInst<dag outs, dag ins, list<dag> patterns> : Instruction {
  let Namespace = "Epiphany";

  let OutOperandList = outs;
  let InOperandList= ins;
  let Pattern = patterns;
  let isCodeGenOnly = 1;
  let isPseudo = 1;
  let Size = 4;
}

// As above, this will be a single A64 instruction, but we can actually give the
// expansion in TableGen.
class A64PseudoExpand<dag outs, dag ins, list<dag> patterns, dag Result>
  : PseudoInst<outs, ins, patterns>,
    PseudoInstExpansion<Result>;


class EP1INST<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin>  : Instruction {

field bits<32> Inst =0 ;
  // LLVM-level model of the Epiphany/A64 distinction.
  let Namespace = "Epiphany";
  let Size = 4;

  // Set the templated fields
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = patterns;
  let Itinerary = itin;
}

class EP2INST<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin>  : Instruction {

field bits<32> Inst =0 ;
  // LLVM-level model of the Epiphany/A64 distinction.
  let Namespace = "Epiphany";
  let Size = 4;

  // Set the templated fields
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = patterns;
  let Itinerary = itin;
}
class EP3INST<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin>  : Instruction {

field bits<32> Inst =0 ;
  // LLVM-level model of the Epiphany/A64 distinction.
  let Namespace = "Epiphany";
  let Size = 4;

  // Set the templated fields
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = patterns;
  let Itinerary = itin;
}

class EP4INST<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin>  : Instruction {

field bits<32> Inst =0 ;
  // LLVM-level model of the Epiphany/A64 distinction.
  let Namespace = "Epiphany";
  let Size = 4;

  // Set the templated fields
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = patterns;
  let Itinerary = itin;
}