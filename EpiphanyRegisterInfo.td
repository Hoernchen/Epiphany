//===- EpiphanyRegisterInfo.td - ARM Register defs ----------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains declarations that describe the Epiphany register file
//
//===----------------------------------------------------------------------===//

let Namespace = "Epiphany" in {
def sub_even : SubRegIndex;
def sub_odd : SubRegIndex;
}

// Registers are identified with 5-bit ID numbers.
class EpiphanyReg<bits<16> enc, string n> : Register<n> {
  let HWEncoding = enc;
  let Namespace = "Epiphany";
}

class EpiphanyRegWithSubs<bits<16> enc, string n, list<Register> subregs = []>  : EpiphanyReg<enc, n> {
  let SubRegs = subregs;
  let CoveredBySubRegs = 1;
}

//===----------------------------------------------------------------------===//
//  Integer registers: w0-w30, wzr, wsp, x0-x30, xzr, sp
//===----------------------------------------------------------------------===//

foreach Index = 0-12 in {
	def R#Index : EpiphanyReg< Index, "r"#Index>, DwarfRegNum<[Index]>;
}

def SP : EpiphanyReg<13, "sp">, DwarfRegNum<[13]>;
def LR : EpiphanyReg<14, "lr">, DwarfRegNum<[14]>;

foreach Index = 15-63 in {
  def R#Index : EpiphanyReg< Index, "r"#Index>, DwarfRegNum<[Index]>;
}

let SubRegIndices = [sub_even, sub_odd] in {
def D0  : EpiphanyRegWithSubs<0,  "r0", [R0,   R1]>, DwarfRegNum<[0,1]>;
def D1  : EpiphanyRegWithSubs<2,  "r2", [R2,   R3]>, DwarfRegNum<[2,3]>;
def D2  : EpiphanyRegWithSubs<4,  "r4", [R4,   R5]>, DwarfRegNum<[4,5]>;
def D3  : EpiphanyRegWithSubs<6,  "r6", [R6,   R7]>, DwarfRegNum<[6,7]>;
def D4  : EpiphanyRegWithSubs<8,  "r8", [R8,   R9]>, DwarfRegNum<[8,9]>;
def D5  : EpiphanyRegWithSubs<10,  "r10", [R10,   R11]>, DwarfRegNum<[10,11]>;

def D6  : EpiphanyRegWithSubs<12,  "r12", [R12,   SP]>, DwarfRegNum<[12,13]>;
def D7  : EpiphanyRegWithSubs<14,  "r14", [LR,   R15]>, DwarfRegNum<[14,15]>;

def D8  : EpiphanyRegWithSubs<16,  "r16", [R16,   R17]>, DwarfRegNum<[16,17]>;
def D9  : EpiphanyRegWithSubs<18,  "r18", [R18,   R19]>, DwarfRegNum<[18,19]>;
def D10  : EpiphanyRegWithSubs<20,  "r20", [R20,   R21]>, DwarfRegNum<[20,21]>;
def D11  : EpiphanyRegWithSubs<22,  "r22", [R22,   R23]>, DwarfRegNum<[22,23]>;
def D12  : EpiphanyRegWithSubs<24,  "r24", [R24,   R25]>, DwarfRegNum<[24,25]>;
def D13  : EpiphanyRegWithSubs<26,  "r26", [R26,   R27]>, DwarfRegNum<[26,27]>;
def D14  : EpiphanyRegWithSubs<28,  "r28", [R28,   R29]>, DwarfRegNum<[28,29]>;
def D15  : EpiphanyRegWithSubs<30,  "r30", [R30,   R31]>, DwarfRegNum<[30,31]>;
def D16  : EpiphanyRegWithSubs<32,  "r32", [R32,   R33]>, DwarfRegNum<[32,33]>;
def D17  : EpiphanyRegWithSubs<34,  "r34", [R34,   R35]>, DwarfRegNum<[34,35]>;
def D18  : EpiphanyRegWithSubs<36,  "r36", [R36,   R37]>, DwarfRegNum<[36,37]>;
def D19  : EpiphanyRegWithSubs<38,  "r38", [R38,   R39]>, DwarfRegNum<[38,39]>;
def D20  : EpiphanyRegWithSubs<40,  "r40", [R40,   R41]>, DwarfRegNum<[40,41]>;
def D21  : EpiphanyRegWithSubs<42,  "r42", [R42,   R43]>, DwarfRegNum<[42,43]>;
def D22  : EpiphanyRegWithSubs<44,  "r44", [R44,   R45]>, DwarfRegNum<[44,45]>;
def D23  : EpiphanyRegWithSubs<46,  "r46", [R46,   R47]>, DwarfRegNum<[46,47]>;
def D24  : EpiphanyRegWithSubs<48,  "r48", [R48,   R49]>, DwarfRegNum<[48,49]>;
def D25  : EpiphanyRegWithSubs<50,  "r50", [R50,   R51]>, DwarfRegNum<[50,51]>;
def D26  : EpiphanyRegWithSubs<52,  "r52", [R52,   R53]>, DwarfRegNum<[52,53]>;
def D27  : EpiphanyRegWithSubs<54,  "r54", [R54,   R55]>, DwarfRegNum<[54,55]>;
def D28  : EpiphanyRegWithSubs<56,  "r56", [R56,   R57]>, DwarfRegNum<[56,57]>;
def D29  : EpiphanyRegWithSubs<58,  "r58", [R58,   R59]>, DwarfRegNum<[58,59]>;
def D30  : EpiphanyRegWithSubs<60,  "r60", [R60,   R61]>, DwarfRegNum<[60,61]>;
// def D31  : EpiphanyRegWithSubs<62,  "d31", [R62,   R63]>, DwarfRegNum<[62,63]>;
}

// Note that the order of registers is important for the Disassembler here:
// tablegen uses it to form MCRegisterClass::getRegister, which we assume can
// take an encoding value.
def GPR32 : RegisterClass<"Epiphany", [i32], 32, (add (sequence "R%u", 0, 12), LR, SP, (sequence "R%u", 15, 63))> {
}

def FPR32 : RegisterClass<"Epiphany", [f32], 32, (add GPR32)> {
}

def DPR64 : RegisterClass<"Epiphany", [i64,f64], 64, (sequence "D%u", 0, 30)> {
	let CopyCost = 2;
}

// Flags register
def NZCV : Register<"nzcv"> {
  let Namespace = "Epiphany";
}

def FlagClass : RegisterClass<"Epiphany", [i32], 32, (add NZCV)> {
  let CopyCost = -1;
  let isAllocatable = 0;
}
