//==-- EpiphanyCallingConv.td - Calling Conventions for ARM ----*- tblgen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for Epiphany architecture.
//===----------------------------------------------------------------------===//

def CC_A64_APCS : CallingConv<[
  // Put ByVal arguments directly on the stack. 
  CCIfByVal<CCPassByVal<4, 4>>,

  //CCIfType<[v1i64, v2i32, v4i16, v8i8, v2f32], CCBitConvertToType<f64>>
  // CCIfType<[f32],  CCAssignToReg<[S0, S1, S2, S3, S4, S5, S6, S7]>>,
  CCIfType<[i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32,f32], CCAssignToReg<[R0, R1, R2, R3]>>,
  // CCIfType<[i64,f64], CCAssignToReg<[D0, D1]>>,
  CCIfType<[i32,f32], CCAssignToStack<4, 4>>
  // CCIfType<[i64,f64], CCAssignToStack<8, 8>>
]>;

def CSR_PCS : CalleeSavedRegs<(add LR, (sequence "R%u", 43, 32),(sequence "R%u", 11, 4))>;
