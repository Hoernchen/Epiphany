//===----- EpiphanyInstrInfo.td - Epiphany Instruction Info ----*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Epiphany scalar instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

include "EpiphanyInstrFormats.td"

//===----------------------------------------------------------------------===//
// Target-specific ISD nodes and profiles
//===----------------------------------------------------------------------===//

//(outs rd), ins (rd, rn, rm, is_sub_flag)
def SDT_fm_a_s : SDTypeProfile<1, 4, [SDTCisSameAs<0, 1>, SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>, SDTCisFP<0>, SDTCisVT<4, i32>]>;
def EPIfmas        : SDNode<"EpiphanyISD::FM_A_S", SDT_fm_a_s>;

def SDT_A64ret : SDTypeProfile<0, 0, []>;
def A64ret : SDNode<"EpiphanyISD::Ret", SDT_A64ret, [SDNPHasChain,
                                                    SDNPOptInGlue,
                                                    SDNPVariadic]>;

// (ins NZCV, Condition, Dest)
def SDT_A64br_cc : SDTypeProfile<0, 3, [SDTCisVT<0, i32>]>;
def A64br_cc : SDNode<"EpiphanyISD::BR_CC", SDT_A64br_cc, [SDNPHasChain]>;

// (outs Result), (ins NZCV, IfTrue, IfFalse, Condition)
def SDT_A64select_cc : SDTypeProfile<1, 4, [SDTCisVT<1, i32>,
                                            SDTCisSameAs<0, 2>,
                                            SDTCisSameAs<2, 3>]>;
def A64select_cc : SDNode<"EpiphanyISD::SELECT_CC", SDT_A64select_cc>;

// (outs NZCV), (ins LHS, RHS, Condition)
def SDT_A64setcc : SDTypeProfile<1, 3, [SDTCisVT<0, i32>,
                                        SDTCisSameAs<1, 2>]>;
def A64setcc : SDNode<"EpiphanyISD::SETCC", SDT_A64setcc>;


// (outs GPR32), (ins)
//def A64threadpointer : SDNode<"EpiphanyISD::THREAD_POINTER", SDTPtrLeaf>;

// A64 compares don't care about the cond really (they set all flags) so a
// simple binary operator is useful.
def A64cmp : PatFrag<(ops node:$lhs, node:$rhs),  (A64setcc node:$lhs, node:$rhs, cond)>;



// There are two layers of indirection here, driven by the following
// considerations.
//     + TableGen does not know CodeModel or Reloc so that decision should be
//       made for a variable/address at ISelLowering.
//     + The output of ISelLowering should be selectable (hence the Wrapper,
//       rather than a bare target opcode)
def SDTEpiphanyWrapper : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>,
                                             SDTCisSameAs<1, 2>,
                                             SDTCisVT<3, i32>,
                                             SDTCisPtrTy<0>]>;

def A64WrapperSmall : SDNode<"EpiphanyISD::WrapperSmall", SDTEpiphanyWrapper>;


//===----------------------------------------------------------------------===//
// Call sequence pseudo-instructions
//===----------------------------------------------------------------------===//


def SDT_EpiphanyCall : SDTypeProfile<0, -1, [SDTCisPtrTy<0>]>;
def EpiphanyCall : SDNode<"EpiphanyISD::Call", SDT_EpiphanyCall,
                     [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

def SDT_EpiphanyCallSeqStart : SDCallSeqStart<[ SDTCisPtrTy<0> ]>;
def Epiphanycallseq_start : SDNode<"ISD::CALLSEQ_START", SDT_EpiphanyCallSeqStart,
                                  [SDNPHasChain, SDNPOutGlue]>;

def SDT_EpiphanyCallSeqEnd   : SDCallSeqEnd<[ SDTCisPtrTy<0>, SDTCisPtrTy<1> ]>;
def Epiphanycallseq_end : SDNode<"ISD::CALLSEQ_END",   SDT_EpiphanyCallSeqEnd,
                                [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;



// These pseudo-instructions have special semantics by virtue of being passed to
// the InstrInfo constructor. CALLSEQ_START/CALLSEQ_END are produced by
// LowerCall to (in our case) tell the back-end about stack adjustments for
// arguments passed on the stack. Here we select those markers to
// pseudo-instructions which explicitly set the stack, and finally in the
// RegisterInfo we convert them to a true stack adjustment.
let Defs = [SP], Uses = [SP] in {
  def ADJCALLSTACKDOWN : PseudoInst<(outs), (ins i32imm:$amt),
                                    [(Epiphanycallseq_start timm:$amt)]>;

  def ADJCALLSTACKUP : PseudoInst<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                 [(Epiphanycallseq_end timm:$amt1, timm:$amt2)]>;
}

//===----------------------------------------------------------------------===//
// Logical (register) instructions
//===----------------------------------------------------------------------===//
let Defs = [NZCV] in {
	def ANDrr : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, GPR32:$Rm),"and\t$Rd, $Rn, $Rm",[(set GPR32:$Rd, (and GPR32:$Rn, GPR32:$Rm))],NoItinerary>;
	def ORRrr : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, GPR32:$Rm),"orr\t$Rd, $Rn, $Rm",[(set GPR32:$Rd, (or GPR32:$Rn, GPR32:$Rm))],NoItinerary>;
	def EORrr : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, GPR32:$Rm),"eor\t$Rd, $Rn, $Rm",[(set GPR32:$Rd, (xor GPR32:$Rn, GPR32:$Rm))],NoItinerary>;
}
//===----------------------------------------------------------------------===//
// Add-subtract (immediate) instructions
//===----------------------------------------------------------------------===//

def neg_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(-N->getZExtValue(), MVT::i32);
}]>;


 let PrintMethod = "printAddSubImmOperand" in {
    def addsubimm_i32_normal : Operand<i32>, ImmLeaf<i32, [{ return Imm >= -1024 && Imm <= 1023; }]>;
    def addsubimm_i32_special_minus : Operand<i32>, ImmLeaf<i32, [{ return Imm == 1024; }], neg_XFORM>;
  }

let Defs = [NZCV] in {
def ADDri : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, addsubimm_i32_normal:$Imm12),"add\t$Rd, $Rn, $Imm12",[/*(set GPR32:$Rd, (addc GPR32:$Rn, imm:$Imm12))*/],NoItinerary>;
def SUBri : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, addsubimm_i32_normal:$Imm12),"sub\t$Rd, $Rn, $Imm12",[/*(set GPR32:$Rd, (subc GPR32:$Rn, imm:$Imm12))*/],NoItinerary>;

	let isCompare = 1 in{
		def SUBri_cmp : EP3INST<(outs),(ins GPR32:$Rn, addsubimm_i32_normal:$Imm12),"cmp\tR63, $Rn, $Imm12",[/*(set NZCV, (A64setcc GPR32:$Rn, imm:$Imm12, cond))*/],NoItinerary>;
	}
}

// special case: add +1024 -> sub -1024 and sub +1024 -> add -1024
def : Pat<(add GPR32:$Rn, addsubimm_i32_special_minus:$Imm12),(SUBri GPR32:$Rn, addsubimm_i32_special_minus:$Imm12)>;
def : Pat<(sub GPR32:$Rn, addsubimm_i32_special_minus:$Imm12),(ADDri GPR32:$Rn, addsubimm_i32_special_minus:$Imm12)>;

def : Pat<(add GPR32:$Rn, addsubimm_i32_normal:$Imm12),(ADDri GPR32:$Rn, addsubimm_i32_normal:$Imm12)>;
def : Pat<(addc GPR32:$Rn, addsubimm_i32_normal:$Imm12),(ADDri GPR32:$Rn, addsubimm_i32_normal:$Imm12)>;


def : Pat<(sub GPR32:$Rn, addsubimm_i32_normal:$Imm12),(SUBri GPR32:$Rn, addsubimm_i32_normal:$Imm12)>;
def : Pat<(subc GPR32:$Rn, addsubimm_i32_normal:$Imm12),(SUBri GPR32:$Rn, addsubimm_i32_normal:$Imm12)>;

// def : Pat<(A64setcc GPR32:$Rn, addsubimm_i32_normal:$Imm12, cond),(ADDri_cmp GPR32:$Rn, addsubimm_i32_normal:$Imm12)>;
def : Pat<(A64setcc GPR32:$Rn, addsubimm_i32_normal:$Imm12, cond),(SUBri_cmp GPR32:$Rn, addsubimm_i32_normal:$Imm12)>;

//===----------------------------------------------------------------------===//
// Add-subtract ( register) instructions
//===----------------------------------------------------------------------===//

let Defs = [NZCV] in {
def ADDrr : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, GPR32:$Rm),"add\t$Rd, $Rn, $Rm",[(set GPR32:$Rd, (addc GPR32:$Rn, (i32 GPR32:$Rm)))],NoItinerary>;
def SUBrr : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, GPR32:$Rm),"sub\t$Rd, $Rn, $Rm",[(set GPR32:$Rd, (subc GPR32:$Rn, (i32 GPR32:$Rm)))],NoItinerary>;
	let isCompare = 1 in {
		def CMPrr : EP3INST<(outs),(ins GPR32:$Rn, GPR32:$Rm),"cmp\tR63, $Rn, $Rm",[(set NZCV, (A64cmp GPR32:$Rn, (i32 GPR32:$Rm)))],NoItinerary>;
	}
	// let Rd = 0b11111, isCompare = 1 in {
	// defm CMPw : addsub_exts<0b0, 0b1, 0b1, "cmp\t", SetNZCV<A64cmp>, (outs), GPR32>;
	// }
}

def : Pat<(add GPR32:$Rn, (i32 GPR32:$Rm)), (ADDrr GPR32:$Rn, GPR32:$Rm)>;
def : Pat<(sub GPR32:$Rn, (i32 GPR32:$Rm)), (SUBrr GPR32:$Rn, GPR32:$Rm)>;
def : Pat<(A64cmp GPR32:$Rn, (i32 GPR32:$Rm)), (CMPrr GPR32:$Rn, GPR32:$Rm)>;

//===----------------------------------------------------------------------===//
// Data Processing (1 source) instructions
//===----------------------------------------------------------------------===//
def REVr : EP2INST<(outs GPR32:$Rd),(ins GPR32:$Rn),"rev\t$Rd, $Rn",[(set GPR32:$Rd, (bswap GPR32:$Rn))],NoItinerary>;

//===----------------------------------------------------------------------===//
// Data Processing (2 sources) instructions
//===----------------------------------------------------------------------===//

def shifts_5bit  : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 31 }]>;

let Defs = [NZCV] in {
	def LSLri : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, shifts_5bit:$UImm5),"lsl\t$Rd, $Rn, $UImm5",[(set GPR32:$Rd, (shl GPR32:$Rn, (i32 imm:$UImm5)) )],NoItinerary>;
	def LSRri : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, shifts_5bit:$UImm5),"lsr\t$Rd, $Rn, $UImm5",[(set GPR32:$Rd, (srl GPR32:$Rn, (i32 imm:$UImm5)) )],NoItinerary>;
	def ASRri : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, shifts_5bit:$UImm5),"asr\t$Rd, $Rn, $UImm5",[(set GPR32:$Rd, (sra GPR32:$Rn, (i32 imm:$UImm5)) )],NoItinerary>;

	def LSLrr : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, GPR32:$Rm),"lsl\t$Rd, $Rn, $Rm",[(set GPR32:$Rd, (shl GPR32:$Rn, GPR32:$Rm))],NoItinerary>;
	def LSRrr : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, GPR32:$Rm),"lsr\t$Rd, $Rn, $Rm",[(set GPR32:$Rd, (srl GPR32:$Rn, GPR32:$Rm))],NoItinerary>;
	def ASRrr : EP3INST<(outs GPR32:$Rd),(ins GPR32:$Rn, GPR32:$Rm),"asr\t$Rd, $Rn, $Rm",[(set GPR32:$Rd, (sra GPR32:$Rn, GPR32:$Rm))],NoItinerary>;
}
//===----------------------------------------------------------------------===//
// Floating-point compare instructions
//===----------------------------------------------------------------------===//
let Defs = [NZCV] in {
	def FCMPss : EP3INST<(outs), (ins FPR32:$Rn, FPR32:$Rm), "fcmp\t$Rn, $Rm", [(set NZCV, (A64cmp (f32 FPR32:$Rn), FPR32:$Rm))], NoItinerary> {}
	  
	let isCommutable = 1 in {
		def FMUL_ss   : EP3INST<(outs FPR32:$Rd),(ins FPR32:$Rn, FPR32:$Rm),"fmul\t$Rd, $Rn, $Rm",[(set (f32 FPR32:$Rd), (fmul FPR32:$Rn, FPR32:$Rm))],NoItinerary>;
		def FADD_ss   : EP3INST<(outs FPR32:$Rd),(ins FPR32:$Rn, FPR32:$Rm),"fadd\t$Rd, $Rn, $Rm",[(set (f32 FPR32:$Rd), (fadd FPR32:$Rn, FPR32:$Rm))],NoItinerary>;
	}
	def FSUB_ss   : EP3INST<(outs FPR32:$Rd),(ins FPR32:$Rn, FPR32:$Rm),"fsub\t$Rd, $Rn, $Rm",[(set (f32 FPR32:$Rd), (fsub FPR32:$Rn, FPR32:$Rm))],NoItinerary>;
	  
	let Constraints = "$Rd = $Ra" in {
		def FMADDsss : EP3INST<(outs FPR32:$Rd),(ins FPR32:$Ra, FPR32:$Rn, FPR32:$Rm),"fmadd\t$Rd, $Rn, $Rm",[/*(set FPR32:$Rd, (fadd FPR32:$Ra, (fmul FPR32:$Rn, FPR32:$Rm)))*/],NoItinerary>;
		def FMSUBsss : EP3INST<(outs FPR32:$Rd),(ins FPR32:$Ra, FPR32:$Rn, FPR32:$Rm),"fmsub\t$Rd, $Rn, $Rm",[/*(set FPR32:$Rd, (fsub FPR32:$Ra, (fmul FPR32:$Rn, FPR32:$Rm)))*/],NoItinerary>;
	}
}

	def : Pat<(EPIfmas FPR32:$Rd, FPR32:$Rn, FPR32:$Rm, 0),(FMADDsss FPR32:$Rd, FPR32:$Rn, FPR32:$Rm)>;
	def : Pat<(EPIfmas FPR32:$Rd, FPR32:$Rn, FPR32:$Rm, 1),(FMSUBsss FPR32:$Rd, FPR32:$Rn, FPR32:$Rm)>;

let Defs = [NZCV] in {
	//===----------------------------------------------------------------------===//
	// Floating-point <-> integer conversion instructions
	//===----------------------------------------------------------------------===//
	def FIXrs : EP3INST<(outs GPR32:$Rd),(ins FPR32:$Rn),"FIX\t$Rd, $Rn",[(set (i32 GPR32:$Rd), (i32 (fp_to_sint FPR32:$Rn)) )],NoItinerary>;
	def FLOATsr : EP3INST<(outs FPR32:$Rd),(ins GPR32:$Rn),"FLOAT\t$Rd, $Rn",[(set (f32 FPR32:$Rd), (f32 (sint_to_fp GPR32:$Rn)) )],NoItinerary>;
	def FABSss : EP3INST<(outs FPR32:$Rd), (ins FPR32:$Rn), "FABS\t$Rd, $Rn",[(set FPR32:$Rd, (fabs FPR32:$Rn))],NoItinerary>;
}
//===----------------------------------------------------------------------===//
// Move wide (immediate) instructions
//===----------------------------------------------------------------------===//

def LO16 : SDNodeXForm<imm, [{
  return getImm(N, N->getZExtValue() & 0xFFFF);
}]>;

def HI16 : SDNodeXForm<imm, [{
  return getImm(N, (N->getZExtValue() >> 16) & 0xFFFF);
}]>;

def immZExt16  : PatLeaf<(imm), [{
    return (uint32_t)N->getZExtValue() == (unsigned short)N->getZExtValue();
}], LO16>;

def immLow16Zero : PatLeaf<(imm), [{
  int64_t Val = N->getSExtValue();
  return isInt<32>(Val) && !(Val & 0xffff);
}]>;

def fmov32_operand : Operand<f32>, PatLeaf<(f32 fpimm), [{ return EpiphanyImms::isFPImm(N->getValueAPF()); }]> { let PrintMethod = "printFPImmOperand";}


let isMoveImm = 1, isAsCheapAsAMove = 1, hasSideEffects  = 0 in {
	let Constraints = "$src = $rt" in {
			def MOVTri : EP2INST<(outs GPR32:$rt), (ins GPR32:$src, i32imm:$imm16),"movt\t$rt, $imm16",[(set GPR32:$rt, (or (and GPR32:$src, 0xffff), immLow16Zero:$imm16))],NoItinerary>;
			def MOVTri_nopat : EP2INST<(outs GPR32:$rt), (ins GPR32:$src, i32imm:$imm16),"movt\t$rt, $imm16",[],NoItinerary>;
			
			def MOVTri_nopat_f : EP2INST<(outs FPR32:$rt), (ins FPR32:$src, fmov32_operand:$imm16),"movt\t$rt, $imm16",[],NoItinerary>;
	}
	def MOVTri_nopat_nodstsrc : EP2INST<(outs GPR32:$rt), (ins i32imm:$imm16),"movt\t$rt, $imm16",[],NoItinerary>;

	def MOVri : EP2INST<(outs GPR32:$rt), (ins i32imm:$imm16), "mov\t$rt, $imm16", [(set GPR32:$rt, immZExt16:$imm16)], NoItinerary>;
	def MOVri_nopat  :  EP2INST<(outs GPR32:$rt),(ins i32imm:$imm16), "mov\t$rt, $imm16", [], NoItinerary>;
	
	def MOVri_nopat_f  :  EP2INST<(outs FPR32:$rt),(ins fmov32_operand:$imm16), "mov\t$rt, $imm16", [], NoItinerary>;
	// def FMOVsi  : EP2INST<(outs FPR32:$Rd), (ins fmov32_operand:$Imm8), "fmov\t$Rd, $Imm8", [], NoItinerary>;
}

//Arbitrary immediates - unfortunately selecting those instead of the rr version fails, so -> code.
// def : Pat<(i32 imm:$imm), (MOVTri_nopat (MOVri_nopat (imm:$imm)),(imm:$imm))>;		
// def : Pat<(f32 fpimm:$imm), (MOVTri_nopat_f (MOVri_nopat_f (fmov32_operand:$imm)),(fmov32_operand:$imm))>;		
def : Pat<(or GPR32:$src, 0xffff0000), (MOVTri_nopat GPR32:$src, 0xffff)>;


class BitconvertPat<ValueType DstVT, ValueType SrcVT, RegisterClass DstRC,RegisterClass SrcRC> : Pat<(DstVT (bitconvert (SrcVT SrcRC:$src))), (COPY_TO_REGCLASS SrcRC:$src, DstRC)>;
def : BitconvertPat<f32, i32, FPR32, GPR32>;
def : BitconvertPat<i32, f32, GPR32, FPR32>;

//===----------------------------------------------------------------------===//
// MOVrr
//===----------------------------------------------------------------------===//
let hasSideEffects  = 0 in{
def MOVww : EP2INST<(outs GPR32:$Rd),(ins GPR32:$Rn),"mov\t$Rd, $Rn",[(set GPR32:$Rd, GPR32:$Rn)],NoItinerary>;
def MOVss : EP2INST<(outs FPR32:$Rd),(ins FPR32:$Rn),"mov\t$Rd, $Rn",[(set FPR32:$Rd, FPR32:$Rn)],NoItinerary>;
// def FMOVws : EP3INST<(outs GPR32:$Rd),(ins FPR32:$Rn),"fmov\t$Rd, $Rn",[(set (i32 GPR32:$Rd), (i32 (bitconvert (f32 FPR32:$Rn))) )],NoItinerary>;
// def FMOVsw : EP3INST<(outs FPR32:$Rd),(ins GPR32:$Rn),"fmov\t$Rd, $Rn",[(set (f32 FPR32:$Rd), (f32 (bitconvert (i32 GPR32:$Rn))) )],NoItinerary>;
}


def cond_code_op : Operand<i32> {
  let PrintMethod = "printCondCodeOperand";
}

// ins true, false, cond -> we prepend a "mov  $Rd, $Rfalse" and make sure that the output ends up in the same $Rd
// this will create a redundant move in case $Rd already happens to be $Rfalse... but we only know this after RA, so we'll fix this in a post-RA pass
  let Uses = [NZCV], Constraints = "$Rd = $Rm" in {
    def MOVCCrr : EP4INST<(outs GPR32:$Rd), (ins GPR32:$Rn, GPR32:$Rm, cond_code_op:$Cond), "mov$Cond\t$Rd, $Rn", [], NoItinerary>;
	def MOVCCss : EP4INST<(outs FPR32:$Rd), (ins FPR32:$Rn, FPR32:$Rm, cond_code_op:$Cond), "mov$Cond\t$Rd, $Rn", [], NoItinerary>;
 } 
 def : Pat<(A64select_cc NZCV, GPR32:$Rn, GPR32:$Rm, (i32 imm:$Cond)), (MOVCCrr GPR32:$Rn, (MOVww GPR32:$Rm), (i32 imm:$Cond))>;
 def : Pat<(A64select_cc NZCV, FPR32:$Rn, FPR32:$Rm, (i32 imm:$Cond)), (MOVCCss FPR32:$Rn, (MOVss FPR32:$Rm), (i32 imm:$Cond))>;
 

 //===----------------------------------------------------------------------===//
// Compare and branch (immediate)
//===----------------------------------------------------------------------===//
def bcc_bimm_target : Operand<OtherVT> {
  // This label is a 19-bit offset from PC, scaled by the instruction-width: 4.
  let PrintMethod = "printLabelOperand<24, 2>";
  let OperandType = "OPERAND_PCREL";
}

//===----------------------------------------------------------------------===//
// Conditional branch (immediate) instructions
//===----------------------------------------------------------------------===//
def cond_code : Operand<i32>, ImmLeaf<i32, [{  return Imm >= 0 && Imm <= 15;}]> {  let PrintMethod = "printCondCodeOperand";}

def Bcc : EP2INST<(outs),(ins cond_code:$Cond, bcc_bimm_target:$Label),"b$Cond $Label",[(A64br_cc NZCV, (i32 imm:$Cond), bb:$Label)],NoItinerary>{
  let Uses = [NZCV];
  let isBranch = 1;
  let isTerminator = 1;
}

//===----------------------------------------------------------------------===//
// Unconditional branch (immediate) instructions
//===----------------------------------------------------------------------===//
def blimm_target : Operand<i32> {
  // This label is a 26-bit offset from PC, scaled by the instruction-width: 4.
  let PrintMethod = "printLabelOperand<24, 2>";
  let OperandType = "OPERAND_PCREL";
}
  
let isBranch = 1 in {
  def Bimm : EP1INST<(outs), (ins bcc_bimm_target:$Label),"b\t$Label", [(br bb:$Label)],NoItinerary> {
    let isTerminator = 1;
    let isBarrier = 1;
  }

  def BLimm : EP1INST<(outs), (ins blimm_target:$Label),"bl\t$Label", [(EpiphanyCall tglobaladdr:$Label)],NoItinerary> {
    let isCall = 1;
    let Defs = [LR];
  }
}

def : Pat<(EpiphanyCall texternalsym:$Label), (BLimm texternalsym:$Label)>;

//===----------------------------------------------------------------------===//
// Unconditional branch (register) instructions
//===----------------------------------------------------------------------===//
// Contains: BR, BLR, RET, ERET, DRP.

// Most of the notional opcode fields in the A64I_Breg format are fixed in A64
// at the moment.
class A64I_BregImpl<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin = NoItinerary>
  : EP1INST<outs, ins, asmstr, patterns, itin> {
  let isBranch         = 1;
  let isIndirectBranch = 1;
}

// Note that these are not marked isCall or isReturn because as far as LLVM is
// concerned they're not. "ret" is just another jump unless it has been selected
// by LLVM as the function's return.

let isBranch = 1 in {
  def JRx : A64I_BregImpl<(outs), (ins GPR32:$Rn),"jr\t$Rn", [(brind GPR32:$Rn)]> {
    let isBarrier = 1;
    let isTerminator = 1;
  }

  def JALRx : A64I_BregImpl<(outs), (ins GPR32:$Rn),"jalr\t$Rn", [(EpiphanyCall GPR32:$Rn)]> {
    let isBarrier = 0;
    let isCall = 1;
    let Defs = [LR];
  }

  def RETx : A64I_BregImpl<(outs), (ins GPR32:$Rn),"rts\t$Rn", []> {
    let isBarrier = 1;
    let isTerminator = 1;
    let isReturn = 1;
  }

  // Create a separate pseudo-instruction for codegen to use so that we don't
  // flag LR as used in every function. It'll be restored before the RET by the
  // epilogue if it's legitimately used.
  def RET : A64PseudoExpand<(outs), (ins), [(A64ret)], (RETx (ops LR))> {
    let isTerminator = 1;
    let isBarrier = 1;
    let isReturn = 1;
  }
}

def RETAlias : InstAlias<"rts", (RETx LR)>;


//===----------------------------------------------------------------------===//
// Address generation patterns
//===----------------------------------------------------------------------===//

class ADRP_ADD<SDNode addrop> : Pat<(A64WrapperSmall addrop:$Hi16, addrop:$Lo16, (i32 imm)),  (MOVTri_nopat (MOVri_nopat (addrop:$Lo16)),(addrop:$Hi16))>;

def : ADRP_ADD<tblockaddress>;
def : ADRP_ADD<texternalsym>;
def : ADRP_ADD<tglobaladdr>;
def : ADRP_ADD<tjumptable>;
 
//===----------------------------------------------------------------------===//
// LS stuff
//===----------------------------------------------------------------------===//

def byte_simm11  : Operand<i32>, ComplexPattern<i32, 1, "SelectOffsetUImm11<1>"> {   let PrintMethod = "printOffsetUImm11Operand<1>";  }
def hword_simm11 : Operand<i32>, ComplexPattern<i32, 1, "SelectOffsetUImm11<2>"> {   let PrintMethod = "printOffsetUImm11Operand<2>";  }
def word_simm11  : Operand<i32>, ComplexPattern<i32, 1, "SelectOffsetUImm11<4>"> {   let PrintMethod = "printOffsetUImm11Operand<4>";  }
def dword_simm11  : Operand<i32>, ComplexPattern<i32, 1, "SelectOffsetUImm11<8>"> {   let PrintMethod = "printOffsetUImm11Operand<8>";  }

//===------------------------------
// 2.1 Regular instructions
//===------------------------------

multiclass A64I_LDRSTR_unsigned<string prefix, bits<2> size, bit v,
                                bit high_opc, string asmsuffix,
                                RegisterClass GPR, Operand params> {
  // Unsigned immediate
  def _STR : EP3INST<(outs), (ins GPR:$Rt, GPR32:$Rn, params:$UImm11),"str" # asmsuffix # "\t$Rt, [$Rn, $UImm11]",[], NoItinerary> {
    let mayStore = 1;
  }
  def : InstAlias<"str" # asmsuffix # " $Rt, [$Rn]", (!cast<Instruction>(prefix # "_STR") GPR:$Rt, GPR32:$Rn, 0)>;

  def _LDR : EP3INST<(outs GPR:$Rt), (ins GPR32:$Rn, params:$UImm11),"ldr" #  asmsuffix # "\t$Rt, [$Rn, $UImm11]",[], NoItinerary> {
    let mayLoad = 1;
  }
  def : InstAlias<"ldr" # asmsuffix # " $Rt, [$Rn]", (!cast<Instruction>(prefix # "_LDR") GPR:$Rt, GPR32:$Rn, 0)>;

  // Register offset (four of these: load/store and Wm/Xm).
  def _RO_LDR : EP3INST<(outs GPR:$Rt),(ins GPR32:$Rn, GPR32:$Rm),"ldr" # asmsuffix # "\t$Rt, [$Rn, $Rm]",[], NoItinerary>{
	let mayLoad = 1;
  }
  def : InstAlias<"ldr" # asmsuffix # " $Rt, [$Rn, $Rm]", (!cast<Instruction>(prefix # "_RO_LDR") GPR:$Rt, GPR32:$Rn, GPR32:$Rm)>;


  def _RO_STR : EP3INST<(outs), (ins GPR:$Rt, GPR32:$Rn, GPR32:$Rm),"str" # asmsuffix # "\t$Rt, [$Rn, $Rm]",[], NoItinerary>{
	let mayStore = 1;
  }
  def : InstAlias<"str" # asmsuffix # " $Rt, [$Rn, $Rm]", (!cast<Instruction>(prefix # "_RO_STR") GPR:$Rt, GPR32:$Rn, GPR32:$Rm)>;

  // Post-indexed
  def _PostInd_STR : EP3INST<(outs GPR32:$Rn_wb),(ins GPR:$Rt, GPR32:$Rn, params:$SImm11),"str" # asmsuffix # "\t$Rt, [$Rn], $SImm11",[], NoItinerary> {
    let Constraints = "$Rn = $Rn_wb";
    let mayStore = 1;

    // Decoder only needed for unpredictability checking (FIXME).
  }

  def _PostInd_LDR : EP3INST<(outs GPR:$Rt, GPR32:$Rn_wb),(ins GPR32:$Rn, params:$SImm11),"ldr" # asmsuffix # "\t$Rt, [$Rn], $SImm11",[], NoItinerary> {
    let mayLoad = 1;
    let Constraints = "$Rn = $Rn_wb";
  }

}


defm LS8 : A64I_LDRSTR_unsigned<"LS8", 0b00, 0b0, 0b0, "b", GPR32, byte_simm11>;
defm LS16 : A64I_LDRSTR_unsigned<"LS16", 0b01, 0b0, 0b0, "h", GPR32, hword_simm11>;
defm LS32 : A64I_LDRSTR_unsigned<"LS32", 0b10, 0b0, 0b0, "w", GPR32, word_simm11>;
defm LSFP32 : A64I_LDRSTR_unsigned<"LSFP32", 0b10, 0b1, 0b0, "w", FPR32, word_simm11>;
defm LSFP64 : A64I_LDRSTR_unsigned<"LSFP64", 0b11, 0b1, 0b0, "d", DPR64, dword_simm11>;


//===----------------------------------------------------------------------===//
// LS patterns
//===----------------------------------------------------------------------===//

//===------------------------------
// 1. Basic infrastructural defs
//===------------------------------

// First, some simple classes for !foreach and !subst to use:
class Decls {
  dag pattern;
}

def decls : Decls;
def ALIGN;
def INST;
def OFFSET;
def SHIFT;

// You can't use !subst on an actual immediate, but you *can* use it on an
// operand record that happens to match a single immediate. So we do.
def imm_eq0 : ImmLeaf<i32, [{ return Imm == 0; }]>;
def imm_eq1 : ImmLeaf<i32, [{ return Imm == 1; }]>;
def imm_eq2 : ImmLeaf<i32, [{ return Imm == 2; }]>;
def imm_eq3 : ImmLeaf<i32, [{ return Imm == 3; }]>;
def imm_eq4 : ImmLeaf<i32, [{ return Imm == 4; }]>;

// If the low bits of a pointer are known to be 0 then an "or" is just as good
// as addition for computing an offset. This fragment forwards that check for
// TableGen's use.
def add_like_or : PatFrag<(ops node:$lhs, node:$rhs), (or node:$lhs, node:$rhs),[{  return CurDAG->isBaseWithConstantOffset(SDValue(N, 0));}]>;

// Load/store (unsigned immediate) operations with relocations against global
// symbols (for lo12) are only valid if those symbols have correct alignment
// (since the immediate offset is divided by the access scale, it can't have a
// remainder).
//
// The guaranteed alignment is provided as part of the WrapperSmall
// operation, and checked against one of these.
def any_align   : ImmLeaf<i32, [{ (void)Imm; return true; }]>;
def min_align2  : ImmLeaf<i32, [{ return Imm >= 2; }]>;
def min_align4  : ImmLeaf<i32, [{ return Imm >= 4; }]>;
def min_align8  : ImmLeaf<i32, [{ return Imm >= 8; }]>;
def min_align16 : ImmLeaf<i32, [{ return Imm >= 16; }]>;

// "Normal" load/store instructions can be used on atomic operations, provided
// the ordering parameter is at most "monotonic". Anything above that needs
// special handling with acquire/release instructions.
class simple_load<PatFrag base> : PatFrag<(ops node:$ptr), (base node:$ptr), [{  return cast<AtomicSDNode>(N)->getOrdering() <= Monotonic;}]>;

def atomic_load_simple_i8  : simple_load<atomic_load_8>;
def atomic_load_simple_i16 : simple_load<atomic_load_16>;
def atomic_load_simple_i32 : simple_load<atomic_load_32>;

class simple_store<PatFrag base> : PatFrag<(ops node:$ptr, node:$val), (base node:$ptr, node:$val), [{  return cast<AtomicSDNode>(N)->getOrdering() <= Monotonic;}]>;

def atomic_store_simple_i8  : simple_store<atomic_store_8>;
def atomic_store_simple_i16 : simple_store<atomic_store_16>;
def atomic_store_simple_i32 : simple_store<atomic_store_32>;

//===------------------------------
// 2. SImm11 and SImm9
//===------------------------------

multiclass simm11_pats_byte<dag address, dag Base, dag Offset> {
def : Pat<(atomic_load_simple_i8 address), (LS8_LDR Base, Offset )>;
def : Pat<(atomic_store_simple_i8 address, GPR32:$Rt), (LS8_STR GPR32:$Rt, Base, Offset )>;
def : Pat<(zextloadi8 address), (LS8_LDR Base, Offset )>;
def : Pat<(extloadi8 address), (LS8_LDR Base, Offset )>;
def : Pat<(truncstorei8 GPR32:$Rt, address),(LS8_STR GPR32:$Rt, Base, Offset )>;
}

multiclass simm11_pats_hword<dag address, dag Base, dag Offset> {
def : Pat<(atomic_load_simple_i16 address), (LS16_LDR Base, Offset )>;
def : Pat<(atomic_store_simple_i16 address, GPR32:$Rt), (LS16_STR GPR32:$Rt, Base, Offset )>;
def : Pat<(zextloadi16 address), (LS16_LDR Base, Offset )>;
def : Pat<(extloadi16 address), (LS16_LDR Base, Offset )>;
def : Pat<(truncstorei16 GPR32:$Rt, address),(LS16_STR GPR32:$Rt, Base, Offset )>;
}

multiclass simm11_pats_word<dag address, dag Base, dag Offset> {
def : Pat<(atomic_load_simple_i32 address ), (LS32_LDR Base, Offset )>;
def : Pat<(atomic_store_simple_i32 address , GPR32:$Rt), (LS32_STR GPR32:$Rt, Base, Offset )>;
//32 native
def : Pat<(i32 (load address )), (LS32_LDR Base, Offset )>;
def : Pat<(f32 (load address )), (LSFP32_LDR Base, Offset )>;
def : Pat<(store (i32 GPR32:$Rt), address ), (LS32_STR GPR32:$Rt, Base, Offset )>;
def : Pat<(store (f32 FPR32:$Rt), address ), (LSFP32_STR FPR32:$Rt, Base, Offset )>;
}

def tframeindex_XFORM : SDNodeXForm<frameindex, [{  int FI = cast<FrameIndexSDNode>(N)->getIndex();  return CurDAG->getTargetFrameIndex(FI, MVT::i32);}]>;

///###############################################################################################
// Straightforward patterns of last resort: a pointer with or without an
// appropriate offset.
defm : simm11_pats_byte<(i32 GPR32:$Rn), (i32 GPR32:$Rn), (i32 0)>;
defm : simm11_pats_hword<(i32 GPR32:$Rn), (i32 GPR32:$Rn), (i32 0)>;
defm : simm11_pats_word<(i32 GPR32:$Rn), (i32 GPR32:$Rn), (i32 0)>;

defm : simm11_pats_byte<(add GPR32:$Rn, byte_simm11:$SImm11), (i32 GPR32:$Rn), (i32 byte_simm11:$SImm11)>;
defm : simm11_pats_hword<(add GPR32:$Rn, hword_simm11:$SImm11), (i32 GPR32:$Rn), (i32 hword_simm11:$SImm11)>;
defm : simm11_pats_word<(add GPR32:$Rn, word_simm11:$SImm11), (i32 GPR32:$Rn), (i32 word_simm11:$SImm11)>;
// The offset could be hidden behind an "or", of course:
defm : simm11_pats_byte<(add_like_or GPR32:$Rn, byte_simm11:$SImm11), (i32 GPR32:$Rn), (i32 byte_simm11:$SImm11)>;
defm : simm11_pats_hword<(add_like_or GPR32:$Rn, hword_simm11:$SImm11), (i32 GPR32:$Rn), (i32 hword_simm11:$SImm11)>;
defm : simm11_pats_word<(add_like_or GPR32:$Rn, word_simm11:$SImm11), (i32 GPR32:$Rn), (i32 word_simm11:$SImm11)>;
// Global addresses under the small-absolute model should use these
// instructions. There are ELF relocations specifically for it.
defm : simm11_pats_byte<(A64WrapperSmall tglobaladdr:$Hi, tglobaladdr:$Lo12, any_align), (MOVTri_nopat (MOVri_nopat (tglobaladdr:$Lo12)),(tglobaladdr:$Hi)), (i32 0)>;
defm : simm11_pats_hword<(A64WrapperSmall tglobaladdr:$Hi, tglobaladdr:$Lo12, min_align2), (MOVTri_nopat (MOVri_nopat (tglobaladdr:$Lo12)),(tglobaladdr:$Hi)), (i32 0)>;
defm : simm11_pats_word<(A64WrapperSmall tglobaladdr:$Hi, tglobaladdr:$Lo12, min_align4), (MOVTri_nopat (MOVri_nopat (tglobaladdr:$Lo12)),(tglobaladdr:$Hi)), (i32 0)>;

// External symbols that make it this far should also get standard relocations.
defm : simm11_pats_byte<(A64WrapperSmall texternalsym:$Hi, texternalsym:$Lo12, any_align), (MOVTri_nopat (MOVri_nopat (texternalsym:$Lo12)),(texternalsym:$Hi)), (i32 0)>;
defm : simm11_pats_hword<(A64WrapperSmall texternalsym:$Hi, texternalsym:$Lo12, min_align2), (MOVTri_nopat (MOVri_nopat (texternalsym:$Lo12)),(texternalsym:$Hi)), (i32 0)>;
defm : simm11_pats_word<(A64WrapperSmall texternalsym:$Hi, texternalsym:$Lo12, min_align4), (MOVTri_nopat (MOVri_nopat (texternalsym:$Lo12)),(texternalsym:$Hi)), (i32 0)>;

defm : simm11_pats_byte<(A64WrapperSmall tconstpool:$Hi, tconstpool:$Lo12, any_align), (MOVTri_nopat (MOVri_nopat (tconstpool:$Lo12)),(tconstpool:$Hi)), (i32 0)>;
defm : simm11_pats_hword<(A64WrapperSmall tconstpool:$Hi, tconstpool:$Lo12, min_align2), (MOVTri_nopat (MOVri_nopat (tconstpool:$Lo12)),(tconstpool:$Hi)), (i32 0)>;
defm : simm11_pats_word<(A64WrapperSmall tconstpool:$Hi, tconstpool:$Lo12, min_align4), (MOVTri_nopat (MOVri_nopat (tconstpool:$Lo12)),(tconstpool:$Hi)), (i32 0)>;

// We also want to use simm11 instructions for local variables at the moment.
defm : simm11_pats_byte<(i32 frameindex:$Rn), (tframeindex_XFORM tframeindex:$Rn), (i32 0)>;
defm : simm11_pats_hword<(i32 frameindex:$Rn), (tframeindex_XFORM tframeindex:$Rn), (i32 0)>;
defm : simm11_pats_word<(i32 frameindex:$Rn), (tframeindex_XFORM tframeindex:$Rn), (i32 0)>;
///###############################################################################################


//===------------------------------
// 3. Register offset patterns
//===------------------------------

multiclass regoff_pats<dag address, dag Base, dag Offset> {
					   
def : Pat<(atomic_load_simple_i8  address ), (LS8_RO_LDR Base, Offset)>;
def : Pat<(atomic_load_simple_i16  address ), (LS16_RO_LDR Base, Offset)>;
def : Pat<(atomic_load_simple_i32  address ), (LS32_RO_LDR Base, Offset)>;

def : Pat<(atomic_store_simple_i8  address, GPR32:$Rt), (LS8_RO_STR GPR32:$Rt, Base, Offset)>;
def : Pat<(atomic_store_simple_i16  address, GPR32:$Rt), (LS16_RO_STR GPR32:$Rt, Base, Offset)>;
def : Pat<(atomic_store_simple_i32  address, GPR32:$Rt), (LS32_RO_STR GPR32:$Rt, Base, Offset)>;

def : Pat<(zextloadi8  address ), (LS8_RO_LDR Base, Offset)>;
def : Pat<(zextloadi16  address ), (LS16_RO_LDR Base, Offset)>;

def : Pat<(extloadi8  address ), (LS8_RO_LDR Base, Offset)>;
def : Pat<(extloadi16  address ), (LS16_RO_LDR Base, Offset)>;

def : Pat<(truncstorei8 GPR32:$Rt,  address ),(LS8_RO_STR GPR32:$Rt, Base, Offset)>;
def : Pat<(truncstorei16 GPR32:$Rt,  address ),(LS16_RO_STR GPR32:$Rt, Base, Offset)>;

//32 native
def : Pat<(i32 (load  address) ), (LS32_RO_LDR Base, Offset)>;
def : Pat<(f32 (load  address) ), (LSFP32_RO_LDR Base, Offset)>;

def : Pat<(store (i32 GPR32:$Rt),  address ), (LS32_RO_STR GPR32:$Rt, Base, Offset)>;  
def : Pat<(store (f32 FPR32:$Rt),  address ), (LSFP32_RO_STR FPR32:$Rt, Base, Offset)>;
}

defm : regoff_pats<(add GPR32:$Rn, GPR32:$Rm), (i32 GPR32:$Rn), (i32 GPR32:$Rm)>;
