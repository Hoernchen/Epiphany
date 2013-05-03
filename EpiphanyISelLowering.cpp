//===-- EpiphanyISelLowering.cpp - Epiphany DAG Lowering Implementation -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Epiphany uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "epiphany-isel"
#include "Epiphany.h"
#include "EpiphanyISelLowering.h"
#include "EpiphanyMachineFunctionInfo.h"
#include "EpiphanyTargetMachine.h"
#include "EpiphanyTargetObjectFile.h"
#include "Utils/EpiphanyBaseInfo.h"
#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/CallingConv.h"

using namespace llvm;

static TargetLoweringObjectFile *createTLOF(EpiphanyTargetMachine &TM) {
  const EpiphanySubtarget *Subtarget = &TM.getSubtarget<EpiphanySubtarget>();

    return new EpiphanyLinuxTargetObjectFile();
}


EpiphanyTargetLowering::EpiphanyTargetLowering(EpiphanyTargetMachine &TM)
  : TargetLowering(TM, createTLOF(TM)),
    Subtarget(&TM.getSubtarget<EpiphanySubtarget>()),
    RegInfo(TM.getRegisterInfo()),
    Itins(TM.getInstrItineraryData()) {

  // Scalar register <-> type mapping
  addRegisterClass(MVT::i32, &Epiphany::GPR32RegClass);
  addRegisterClass(MVT::f32, &Epiphany::FPR32RegClass);

  // we do have a reg class, but the only supported operations are loads and stores
  //addRegisterClass(MVT::i64, &Epiphany::DPR64RegClass);
  //addRegisterClass(MVT::f64, &Epiphany::DPR64RegClass);

  computeRegisterProperties();

  setTargetDAGCombine(ISD::FADD);
  setTargetDAGCombine(ISD::FSUB);

  // Epiphany does not have i1 loads, or much of anything for i1 really.
  setLoadExtAction(ISD::SEXTLOAD, MVT::i1, Promote);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i1, Promote);
  setLoadExtAction(ISD::EXTLOAD, MVT::i1, Promote);

  setStackPointerRegisterToSaveRestore(Epiphany::SP);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);

  // We'll lower globals to wrappers for selection.
  setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);

  // A64 instructions have the comparison predicate attached to the user of the
  // result, but having a separate comparison is valuable for matching.
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_CC, MVT::f32, Custom);
  setOperationAction(ISD::SELECT, MVT::i32, Custom);
  setOperationAction(ISD::SELECT, MVT::f32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
  setOperationAction(ISD::BRCOND, MVT::Other, Custom);
  setOperationAction(ISD::SETCC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::f32, Custom);
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::JumpTable, MVT::i32, Custom);
  setOperationAction(ISD::VASTART, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::BlockAddress, MVT::i32, Custom);
  setOperationAction(ISD::ROTL, MVT::i32, Expand);
  setOperationAction(ISD::UREM, MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::SREM, MVT::i32, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::SDIV, MVT::i32, Expand);
  setOperationAction(ISD::UDIV, MVT::i32, Expand);
  setOperationAction(ISD::CTPOP, MVT::i32, Expand);

  setOperationAction(ISD::FNEG, MVT::f32, Custom);

  // Legal floating-point operations.
  setOperationAction(ISD::FMUL, MVT::f32, Legal);
  setOperationAction(ISD::FADD, MVT::f32, Legal);
  setOperationAction(ISD::FSUB, MVT::f32, Legal);


  setOperationAction(ISD::FABS, MVT::f32, Legal);
  setOperationAction(ISD::ConstantFP, MVT::f32, Legal);

  //setOperationAction(ISD::FMA, MVT::f32, Expand); custom combine due to fneg, we have fmsub

  // Illegal floating-point operations.
  setOperationAction(ISD::FDIV, MVT::f32, Expand);
  setOperationAction(ISD::FCOPYSIGN, MVT::f32, Expand);
  setOperationAction(ISD::FCOS, MVT::f32, Expand);
  setOperationAction(ISD::FEXP, MVT::f32, Expand);
  setOperationAction(ISD::FEXP2, MVT::f32, Expand);
  setOperationAction(ISD::FLOG, MVT::f32, Expand);
  setOperationAction(ISD::FLOG2, MVT::f32, Expand);
  setOperationAction(ISD::FLOG10, MVT::f32, Expand);
  setOperationAction(ISD::FPOW, MVT::f32, Expand);
  setOperationAction(ISD::FPOWI, MVT::f32, Expand);
  setOperationAction(ISD::FREM, MVT::f32, Expand);
  setOperationAction(ISD::FSIN, MVT::f32, Expand);


//fix/float
  setOperationAction(ISD::FP_TO_SINT, MVT::i32, Legal);
  setOperationAction(ISD::SINT_TO_FP, MVT::i32, Legal);

  }

EVT EpiphanyTargetLowering::getSetCCResultType(EVT VT) const {
  // It's reasonably important that this value matches the "natural" legal
  // promotion from i1 for scalar types. Otherwise LegalizeTypes can get itself
  // in a twist (e.g. inserting an any_extend which then becomes i32 -> i32).
  if (!VT.isVector()) return MVT::i32;
  return VT.changeVectorElementTypeToInteger();
}

MachineBasicBlock *
EpiphanyTargetLowering::EmitInstrWithCustomInserter(MachineInstr *MI,
                                                 MachineBasicBlock *MBB) const {
  switch (MI->getOpcode()) {
  default: llvm_unreachable("Unhandled instruction with custom inserter");
  //case Epiphany::F128CSEL:
  //  return EmitF128CSEL(MI, MBB);
  }
}


const char *EpiphanyTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case EpiphanyISD::BR_CC:          return "EpiphanyISD::BR_CC";
  case EpiphanyISD::Call:           return "EpiphanyISD::Call";
  case EpiphanyISD::Ret:            return "EpiphanyISD::Ret";
  case EpiphanyISD::SELECT_CC:      return "EpiphanyISD::SELECT_CC";
  case EpiphanyISD::SETCC:          return "EpiphanyISD::SETCC";
  case EpiphanyISD::WrapperSmall:   return "EpiphanyISD::WrapperSmall";
  case EpiphanyISD::FM_A_S:			return "EpiphanyISD::FM_A_S";

  default:                       return NULL;
  }
}

static const uint16_t EpiphanyArgRegs[] = {
  Epiphany::R0, Epiphany::R1, Epiphany::R2, Epiphany::R3
};
static const unsigned NumArgRegs = llvm::array_lengthof(EpiphanyArgRegs);

#include "EpiphanyGenCallingConv.inc"

CCAssignFn *EpiphanyTargetLowering::CCAssignFnForNode(CallingConv::ID CC) const {

  switch(CC) {
  default: llvm_unreachable("Unsupported calling convention");
  case CallingConv::C:
    return CC_A64_APCS;
  }
}

void
EpiphanyTargetLowering::SaveVarArgRegisters(CCState &CCInfo, SelectionDAG &DAG,
                                           DebugLoc DL, SDValue &Chain) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  EpiphanyMachineFunctionInfo *FuncInfo
    = MF.getInfo<EpiphanyMachineFunctionInfo>();

  SmallVector<SDValue, 8> MemOps;

  unsigned FirstVariadicGPR = CCInfo.getFirstUnallocated(EpiphanyArgRegs,
                                                         NumArgRegs);

  unsigned GPRSaveSize = 4 * (NumArgRegs - FirstVariadicGPR);
  int GPRIdx = 0;
  if (GPRSaveSize != 0) {
    GPRIdx = MFI->CreateStackObject(GPRSaveSize, 4, false);

    SDValue FIN = DAG.getFrameIndex(GPRIdx, getPointerTy());

    for (unsigned i = FirstVariadicGPR; i < NumArgRegs; ++i) {
      unsigned VReg = MF.addLiveIn(EpiphanyArgRegs[i], &Epiphany::GPR32RegClass);
      SDValue Val = DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32);
      SDValue Store = DAG.getStore(Val.getValue(1), DL, Val, FIN,
                                   MachinePointerInfo::getStack(i * 4),
                                   false, false, 0);
      MemOps.push_back(Store);
      FIN = DAG.getNode(ISD::ADD, DL, getPointerTy(), FIN,
                        DAG.getConstant(4, getPointerTy()));
    }
  }

  int StackIdx = MFI->CreateFixedObject(4, CCInfo.getNextStackOffset(), true);

  FuncInfo->setVariadicStackIdx(StackIdx);
  FuncInfo->setVariadicGPRIdx(GPRIdx);
  FuncInfo->setVariadicGPRSize(GPRSaveSize);
  FuncInfo->setVariadicFPRIdx(0);
  FuncInfo->setVariadicFPRSize(0);

  if (!MemOps.empty()) {
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, &MemOps[0],
                        MemOps.size());
  }
}


SDValue
EpiphanyTargetLowering::LowerFormalArguments(SDValue Chain,
                                      CallingConv::ID CallConv, bool isVarArg,
                                      const SmallVectorImpl<ISD::InputArg> &Ins,
                                      DebugLoc dl, SelectionDAG &DAG,
                                      SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  EpiphanyMachineFunctionInfo *FuncInfo = MF.getInfo<EpiphanyMachineFunctionInfo>();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  //bool TailCallOpt = MF.getTarget().Options.GuaranteedTailCallOpt;

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(), getTargetMachine(), ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CCAssignFnForNode(CallConv));

  SmallVector<SDValue, 16> ArgValues;

  SDValue ArgValue;
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];

	if (VA.isRegLoc()) {
      MVT RegVT = VA.getLocVT();
      const TargetRegisterClass *RC = getRegClassFor(RegVT);
      unsigned Reg = MF.addLiveIn(VA.getLocReg(), RC);
      ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, RegVT);

	  switch (VA.getLocInfo()) {
      default: llvm_unreachable("Unknown loc info!");
      case CCValAssign::Full: break;
      case CCValAssign::BCvt:
        ArgValue = DAG.getNode(ISD::BITCAST, dl, VA.getValVT(), ArgValue);
        break;
      case CCValAssign::SExt:
        ArgValue = DAG.getNode(ISD::AssertSext, dl, RegVT, ArgValue, DAG.getValueType(VA.getValVT()));
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), ArgValue);
        break;
      case CCValAssign::ZExt:
        ArgValue = DAG.getNode(ISD::AssertZext, dl, RegVT, ArgValue, DAG.getValueType(VA.getValVT()));
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), ArgValue);
        break;
      }

    } else { // VA.isRegLoc()
      assert(VA.isMemLoc());
	// hm. byval?
      int FI = MFI->CreateFixedObject(VA.getLocVT().getSizeInBits()/8, VA.getLocMemOffset(), true);
      SDValue FIN = DAG.getFrameIndex(FI, getPointerTy());
      ArgValue = DAG.getLoad(VA.getLocVT(), dl, Chain, FIN, MachinePointerInfo::getFixedStack(FI),false, false, false, 0);
    }

    InVals.push_back(ArgValue);
  }

  if (isVarArg)
    SaveVarArgRegisters(CCInfo, DAG, dl, Chain);

  unsigned StackArgSize = CCInfo.getNextStackOffset();
  // Even if we're not expected to free up the space, it's useful to know how
  // much is there while considering tail calls (because we can reuse it).
  FuncInfo->setBytesInStackArgArea(StackArgSize);

  return Chain;
}

SDValue
EpiphanyTargetLowering::LowerFNEG(SDValue Op, SelectionDAG &DAG) const{
	DebugLoc dl = Op.getDebugLoc();
	SDValue Operand0 = Op.getOperand(0);
	EVT VT = Op.getValueType();

	// y = fneg(x) -> xor rd, 0x80000000
	SDValue Val = DAG.getConstant(0x80000000, MVT::i32);
	SDValue Arg = DAG.getNode(ISD::BITCAST, dl, MVT::i32, Operand0);
	SDValue Xor = DAG.getNode(ISD::XOR, dl, MVT::i32, Arg, Val);
	return DAG.getNode(ISD::BITCAST, dl, VT, Xor);
}

SDValue
EpiphanyTargetLowering::LowerReturn(SDValue Chain,
                                   CallingConv::ID CallConv, bool isVarArg,
                                   const SmallVectorImpl<ISD::OutputArg> &Outs,
                                   const SmallVectorImpl<SDValue> &OutVals,
                                   DebugLoc dl, SelectionDAG &DAG) const {
  // CCValAssign - represent the assignment of the return value to a location.
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slots.
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 getTargetMachine(), RVLocs, *DAG.getContext());

  // Analyze outgoing return values.
  CCInfo.AnalyzeReturn(Outs, CCAssignFnForNode(CallConv));

  SDValue Flag;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    // PCS: "If the type, T, of the result of a function is such that
    // void func(T arg) would require that arg be passed as a value in a
    // register (or set of registers) according to the rules in 5.4, then the
    // result is returned in the same registers as would be used for such an
    // argument.
    //
    // Otherwise, the caller shall reserve a block of memory of sufficient
    // size and alignment to hold the result. The address of the memory block
    // shall be passed as an additional argument to the function in x8."
    //
    // This is implemented in two places. The register-return values are dealt
    // with here, more complex returns are passed as an sret parameter, which
    // means we don't have to worry about it during actual return.
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Only register-returns should be created by PCS");


    SDValue Arg = OutVals[i];

    // There's no convenient note in the ABI about this as there is for normal
    // arguments, but it says return values are passed in the same registers as
    // an argument would be. I believe that includes the comments about
    // unspecified higher bits, putting the burden of widening on the *caller*
    // for return values.
    switch (VA.getLocInfo()) {
    default: llvm_unreachable("Unknown loc info");
    case CCValAssign::Full: break;
    case CCValAssign::SExt:
    case CCValAssign::ZExt:
    case CCValAssign::AExt:
      // Floating-point values should only be extended when they're going into
      // memory, which can't happen here so an integer extend is acceptable.
      Arg = DAG.getNode(ISD::ANY_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::BCvt:
      Arg = DAG.getNode(ISD::BITCAST, dl, VA.getLocVT(), Arg);
      break;
    }

    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), Arg, Flag);
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain;  // Update chain.

  // Add the flag if we have it.
  if (Flag.getNode())
    RetOps.push_back(Flag);

  return DAG.getNode(EpiphanyISD::Ret, dl, MVT::Other,
                     &RetOps[0], RetOps.size());
}

SDValue
EpiphanyTargetLowering::LowerCall(CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const {
	SelectionDAG &DAG                     = CLI.DAG;
	DebugLoc &dl                          = CLI.DL;
	SmallVector<ISD::OutputArg, 32> &Outs = CLI.Outs;
	SmallVector<SDValue, 32> &OutVals     = CLI.OutVals;
	SmallVector<ISD::InputArg, 32> &Ins   = CLI.Ins;
	SDValue Chain                         = CLI.Chain;
	SDValue Callee                        = CLI.Callee;
	bool &IsTailCall                      = CLI.IsTailCall;
	CallingConv::ID CallConv              = CLI.CallConv;
	bool IsVarArg                         = CLI.IsVarArg;

	MachineFunction &MF = DAG.getMachineFunction();
	EpiphanyMachineFunctionInfo *FuncInfo = MF.getInfo<EpiphanyMachineFunctionInfo>();
	//bool TailCallOpt = MF.getTarget().Options.GuaranteedTailCallOpt;
	bool IsStructRet = !Outs.empty() && Outs[0].Flags.isSRet();


	if (IsTailCall) {
		IsTailCall = IsEligibleForTailCallOptimization(Callee, CallConv, IsVarArg, IsStructRet, MF.getFunction()->hasStructRetAttr(), Outs, OutVals, Ins, DAG);
	}

	SmallVector<CCValAssign, 16> ArgLocs;
	CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), getTargetMachine(), ArgLocs, *DAG.getContext());
	CCInfo.AnalyzeCallOperands(Outs, CCAssignFnForNode(CallConv));

	// On Epiphany (and all other architectures I'm aware of) the most this has to
	// do is adjust the stack pointer.
	unsigned NumBytes = RoundUpToAlignment(CCInfo.getNextStackOffset(), 4);

	Chain = DAG.getCALLSEQ_START(Chain, DAG.getIntPtrConstant(NumBytes, true));

	SDValue StackPtr = DAG.getCopyFromReg(Chain, dl, Epiphany::SP, getPointerTy());

	SmallVector<SDValue, 8> MemOpChains;
	SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;

	for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
		CCValAssign &VA = ArgLocs[i];
		ISD::ArgFlagsTy Flags = Outs[i].Flags;
		SDValue Arg = OutVals[i];

		// Callee does the actual widening, so all extensions just use an implicit
		// definition of the rest of the Loc. Aesthetically, this would be nicer as
		// an ANY_EXTEND, but that isn't valid for floating-point types and this
		// alternative works on integer types too.
		switch (VA.getLocInfo()) {
		default: llvm_unreachable("Unknown loc info!");
		case CCValAssign::Full: break;
		case CCValAssign::SExt:
		case CCValAssign::ZExt:
		case CCValAssign::AExt: {
			// Floating-point arguments only get extended/truncated if they're going
			// in memory, so using the integer operation is acceptable here.
			Arg = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), Arg);
			break;
								}
		case CCValAssign::BCvt:
			Arg = DAG.getNode(ISD::BITCAST, dl, VA.getLocVT(), Arg);
			break;
		}

		if (VA.isRegLoc()) {
			// A normal register (sub-) argument. For now we just note it down because
			// we want to copy things into registers as late as possible to avoid
			// register-pressure (and possibly worse).
			RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
			continue;
		}

		assert(VA.isMemLoc() && "unexpected argument location");

		SDValue PtrOff = DAG.getIntPtrConstant(VA.getLocMemOffset());
		SDValue DstAddr = DAG.getNode(ISD::ADD, dl, getPointerTy(), StackPtr, PtrOff);
		MachinePointerInfo DstInfo = MachinePointerInfo::getStack(VA.getLocMemOffset());


		if (Flags.isByVal()) {
			SDValue SizeNode = DAG.getConstant(Flags.getByValSize(), MVT::i32);
			SDValue Cpy = DAG.getMemcpy(Chain, dl, DstAddr, Arg, SizeNode,
				Flags.getByValAlign(),
				/*isVolatile = */ false,
				/*alwaysInline = */ false,
				DstInfo, MachinePointerInfo(0));
			MemOpChains.push_back(Cpy);
		} else {
			// Normal stack argument, put it where it's needed.
			SDValue Store = DAG.getStore(Chain, dl, Arg, DstAddr, DstInfo, false, false, 0);
			MemOpChains.push_back(Store);
		}
	}

	// The loads and stores generated above shouldn't clash with each
	// other. Combining them with this TokenFactor notes that fact for the rest of
	// the backend.
	if (!MemOpChains.empty())
		Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, &MemOpChains[0], MemOpChains.size());

	// Most of the rest of the instructions need to be glued together; we don't
	// want assignments to actual registers used by a call to be rearranged by a
	// well-meaning scheduler.
	SDValue InFlag;

	for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
		Chain = DAG.getCopyToReg(Chain, dl, RegsToPass[i].first, RegsToPass[i].second, InFlag);
		InFlag = Chain.getValue(1);
	}

	// The linker is responsible for inserting veneers when necessary to put a
	// function call destination in range, so we don't need to bother with a
	// wrapper here.
	if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) {
		const GlobalValue *GV = G->getGlobal();
		Callee = DAG.getTargetGlobalAddress(GV, dl, getPointerTy());
	} else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(Callee)) {
		const char *Sym = S->getSymbol();
		Callee = DAG.getTargetExternalSymbol(Sym, getPointerTy());
	}

	// We produce the following DAG scheme for the actual call instruction:
	//     (EpiphanyCall Chain, Callee, reg1, ..., regn, preserveMask, inflag?
	//
	// Most arguments aren't going to be used and just keep the values live as
	// far as LLVM is concerned. It's expected to be selected as simply "bl
	// callee" (for a direct, non-tail call).
	std::vector<SDValue> Ops;
	Ops.push_back(Chain);
	Ops.push_back(Callee);


	for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i)
		Ops.push_back(DAG.getRegister(RegsToPass[i].first, RegsToPass[i].second.getValueType()));

	// Add a register mask operand representing the call-preserved registers. This
	// is used later in codegen to constrain register-allocation.
	const TargetRegisterInfo *TRI = getTargetMachine().getRegisterInfo();
	const uint32_t *Mask = TRI->getCallPreservedMask(CallConv);
	assert(Mask && "Missing call preserved mask for calling convention");
	Ops.push_back(DAG.getRegisterMask(Mask));

	// If we needed glue, put it in as the last argument.
	if (InFlag.getNode())
		Ops.push_back(InFlag);

	SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);

	Chain = DAG.getNode(EpiphanyISD::Call, dl, NodeTys, &Ops[0], Ops.size());
	InFlag = Chain.getValue(1);

	// Now we can reclaim the stack, just as well do it before working out where
	// our return value is.

	uint64_t CalleePopBytes = /*DoesCalleeRestoreStack(CallConv, TailCallOpt) ? NumBytes :*/ 0;

	Chain = DAG.getCALLSEQ_END(Chain, DAG.getIntPtrConstant(NumBytes, true),
		DAG.getIntPtrConstant(CalleePopBytes, true),
		InFlag);
	InFlag = Chain.getValue(1);


	return LowerCallResult(Chain, InFlag, CallConv, IsVarArg, Ins, dl, DAG, InVals);
}

SDValue
EpiphanyTargetLowering::LowerCallResult(SDValue Chain, SDValue InFlag,
                                      CallingConv::ID CallConv, bool IsVarArg,
                                      const SmallVectorImpl<ISD::InputArg> &Ins,
                                      DebugLoc dl, SelectionDAG &DAG,
                                      SmallVectorImpl<SDValue> &InVals) const {
  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(),
                 getTargetMachine(), RVLocs, *DAG.getContext());
  CCInfo.AnalyzeCallResult(Ins, CCAssignFnForNode(CallConv));

  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign VA = RVLocs[i];

    // Return values that are too big to fit into registers should use an sret
    // pointer, so this can be a lot simpler than the main argument code.
    assert(VA.isRegLoc() && "Memory locations not expected for call return");

    SDValue Val = DAG.getCopyFromReg(Chain, dl, VA.getLocReg(), VA.getLocVT(),
                                     InFlag);
    Chain = Val.getValue(1);
    InFlag = Val.getValue(2);

    switch (VA.getLocInfo()) {
    default: llvm_unreachable("Unknown loc info!");
    case CCValAssign::Full: break;
    case CCValAssign::BCvt:
      Val = DAG.getNode(ISD::BITCAST, dl, VA.getValVT(), Val);
      break;
    case CCValAssign::ZExt:
    case CCValAssign::SExt:
    case CCValAssign::AExt:
      // Floating-point arguments only get extended/truncated if they're going
      // in memory, so using the integer operation is acceptable here.
      Val = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), Val);
      break;
    }

    InVals.push_back(Val);
  }

  return Chain;
}


SDValue EpiphanyTargetLowering::addTokenForArgument(SDValue Chain,
                                                   SelectionDAG &DAG,
                                                   MachineFrameInfo *MFI,
                                                   int ClobberedFI) const {
  SmallVector<SDValue, 8> ArgChains;
  int64_t FirstByte = MFI->getObjectOffset(ClobberedFI);
  int64_t LastByte = FirstByte + MFI->getObjectSize(ClobberedFI) - 1;

  // Include the original chain at the beginning of the list. When this is
  // used by target LowerCall hooks, this helps legalize find the
  // CALLSEQ_BEGIN node.
  ArgChains.push_back(Chain);

  // Add a chain value for each stack argument corresponding
  for (SDNode::use_iterator U = DAG.getEntryNode().getNode()->use_begin(),
         UE = DAG.getEntryNode().getNode()->use_end(); U != UE; ++U)
    if (LoadSDNode *L = dyn_cast<LoadSDNode>(*U))
      if (FrameIndexSDNode *FI = dyn_cast<FrameIndexSDNode>(L->getBasePtr()))
        if (FI->getIndex() < 0) {
          int64_t InFirstByte = MFI->getObjectOffset(FI->getIndex());
          int64_t InLastByte = InFirstByte;
          InLastByte += MFI->getObjectSize(FI->getIndex()) - 1;

          if ((InFirstByte <= FirstByte && FirstByte <= InLastByte) ||
              (FirstByte <= InFirstByte && InFirstByte <= LastByte))
            ArgChains.push_back(SDValue(L, 1));
        }

   // Build a tokenfactor for all the chains.
   return DAG.getNode(ISD::TokenFactor, Chain.getDebugLoc(), MVT::Other,
                      &ArgChains[0], ArgChains.size());
}

static EpiphanyCC::CondCodes IntCCToEpiphanyCC(ISD::CondCode CC) {
  switch (CC) {
  case ISD::SETEQ:  return EpiphanyCC::EQ;
  case ISD::SETGT:  return EpiphanyCC::GT;
  case ISD::SETGE:  return EpiphanyCC::GTE;
  case ISD::SETLT:  return EpiphanyCC::LT;
  case ISD::SETLE:  return EpiphanyCC::LTE;
  case ISD::SETNE:  return EpiphanyCC::NE;
  case ISD::SETUGT: return EpiphanyCC::GTU;
  case ISD::SETUGE: return EpiphanyCC::GTEU;
  case ISD::SETULT: return EpiphanyCC::LTU;
  case ISD::SETULE: return EpiphanyCC::LTEU;
  default: llvm_unreachable("Unexpected condition code");
  }
}

bool EpiphanyTargetLowering::isLegalICmpImmediate(int64_t Val) const {
  // icmp is implemented using adds/subs immediate with a 11bit signed imm
  // Symmetric by using adds/subs
  if (Val < 0)
    Val = -Val;

  // 11bit imm -> 10 bit unsigned + 1 sign bit
  return (Val & ~0x3FF) == 0;
}

SDValue EpiphanyTargetLowering::getSelectableIntSetCC(SDValue LHS, SDValue RHS,
                                        ISD::CondCode CC, SDValue &A64cc,
                                        SelectionDAG &DAG, DebugLoc &dl) const {
  if (ConstantSDNode *RHSC = dyn_cast<ConstantSDNode>(RHS.getNode())) {
    int64_t C = 0;
    EVT VT = RHSC->getValueType(0);
    bool knownInvalid = false;

    // I'm not convinced the rest of LLVM handles these edge cases properly, but
    // we can at least get it right.
    if (isSignedIntSetCC(CC)) {
      C = RHSC->getSExtValue();
    } else if (RHSC->getZExtValue() > INT64_MAX) {
      // A 64-bit constant not representable by a signed 64-bit integer is far
      // too big to fit into a SUBS immediate anyway.
      knownInvalid = true;
    } else {
      C = RHSC->getZExtValue();
    }

    if (!knownInvalid && !isLegalICmpImmediate(C)) {
      // Constant does not fit, try adjusting it by one?
      switch (CC) {
      default: break;
      case ISD::SETLT:
      case ISD::SETGE:
        if (isLegalICmpImmediate(C-1)) {
          CC = (CC == ISD::SETLT) ? ISD::SETLE : ISD::SETGT;
          RHS = DAG.getConstant(C-1, VT);
        }
        break;
      case ISD::SETULT:
      case ISD::SETUGE:
        if (isLegalICmpImmediate(C-1)) {
          CC = (CC == ISD::SETULT) ? ISD::SETULE : ISD::SETUGT;
          RHS = DAG.getConstant(C-1, VT);
        }
        break;
      case ISD::SETLE:
      case ISD::SETGT:
        if (isLegalICmpImmediate(C+1)) {
          CC = (CC == ISD::SETLE) ? ISD::SETLT : ISD::SETGE;
          RHS = DAG.getConstant(C+1, VT);
        }
        break;
      case ISD::SETULE:
      case ISD::SETUGT:
        if (isLegalICmpImmediate(C+1)) {
          CC = (CC == ISD::SETULE) ? ISD::SETULT : ISD::SETUGE;
          RHS = DAG.getConstant(C+1, VT);
        }
        break;
      }
    }
  }

  EpiphanyCC::CondCodes CondCode = IntCCToEpiphanyCC(CC);
  A64cc = DAG.getConstant(CondCode, MVT::i32);
  return DAG.getNode(EpiphanyISD::SETCC, dl, MVT::i32, LHS, RHS,
                     DAG.getCondCode(CC));
}

static EpiphanyCC::CondCodes FPCCToEpiphanyCC(ISD::CondCode CC, bool& invert) {
		EpiphanyCC::CondCodes CondCode = EpiphanyCC::Invalid;
		invert = 0;

		switch (CC) {
		default: llvm_unreachable("Unknown FP condition!");
		case ISD::SETEQ:
		case ISD::SETOEQ: 
		case ISD::SETUEQ: CondCode = EpiphanyCC::BEQ; break;

		case ISD::SETNE:
		case ISD::SETUNE:
		case ISD::SETONE: CondCode = EpiphanyCC::BNE; break;

		case ISD::SETGT:
		case ISD::SETOGT:
		case ISD::SETUGT: CondCode = EpiphanyCC::BLT; invert = 1; break;

		case ISD::SETGE:
		case ISD::SETOGE:
		case ISD::SETUGE: CondCode = EpiphanyCC::BLTE; invert = 1; break;

		case ISD::SETLT:
		case ISD::SETULT:
		case ISD::SETOLT: CondCode = EpiphanyCC::BLT; break;

		case ISD::SETLE:
		case ISD::SETULE:
		case ISD::SETOLE: CondCode = EpiphanyCC::BLTE; break;

		//case ISD::SETO:
		//case ISD::SETUO: ooops. call?
		}
		return CondCode;
}

SDValue
EpiphanyTargetLowering::LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const {
  DebugLoc DL = Op.getDebugLoc();
  EVT PtrVT = getPointerTy();
  const BlockAddress *BA = cast<BlockAddressSDNode>(Op)->getBlockAddress();

  assert(getTargetMachine().getCodeModel() == CodeModel::Small
         && "Only small code model supported at the moment");

  // The most efficient code is PC-relative anyway for the small memory model,
  // so we don't need to worry about relocation model.
  return DAG.getNode(EpiphanyISD::WrapperSmall, DL, PtrVT,
                     DAG.getTargetBlockAddress(BA, PtrVT, 0,
                                               EpiphanyII::MO_HI16),
                     DAG.getTargetBlockAddress(BA, PtrVT, 0,
                                               EpiphanyII::MO_LO16),
                     DAG.getConstant(/*Alignment=*/ 4, MVT::i32));
}


// (BRCOND chain, val, dest)
SDValue
EpiphanyTargetLowering::LowerBRCOND(SDValue Op, SelectionDAG &DAG) const {
  DebugLoc dl = Op.getDebugLoc();
  SDValue Chain = Op.getOperand(0);
  SDValue TheBit = Op.getOperand(1);
  SDValue DestBB = Op.getOperand(2);

  // Epiphany BooleanContents is the default UndefinedBooleanContent, which means
  // that as the consumer we are responsible for ignoring rubbish in higher
  // bits.
  TheBit = DAG.getNode(ISD::AND, dl, MVT::i32, TheBit,
                       DAG.getConstant(1, MVT::i32));

  SDValue A64CMP = DAG.getNode(EpiphanyISD::SETCC, dl, MVT::i32, TheBit,
                               DAG.getConstant(0, TheBit.getValueType()),
                               DAG.getCondCode(ISD::SETNE));

  return DAG.getNode(EpiphanyISD::BR_CC, dl, MVT::Other, Chain,
                     A64CMP, DAG.getConstant(EpiphanyCC::NE, MVT::i32),
                     DestBB);
}

// (BR_CC chain, condcode, lhs, rhs, dest)
SDValue
EpiphanyTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  DebugLoc dl = Op.getDebugLoc();
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue DestBB = Op.getOperand(4);

  if (LHS.getValueType().isInteger()) {
    SDValue A64cc;

    // Integers are handled in a separate function because the combinations of
    // immediates and tests can get hairy and we may want to fiddle things.
    SDValue CmpOp = getSelectableIntSetCC(LHS, RHS, CC, A64cc, DAG, dl);

    return DAG.getNode(EpiphanyISD::BR_CC, dl, MVT::Other,
                       Chain, CmpOp, A64cc, DestBB);
  }

  EpiphanyCC::CondCodes CondCode;
  bool invert;
  SDValue SetCC;
  CondCode = FPCCToEpiphanyCC(CC, invert);
  SDValue A64cc = DAG.getConstant(CondCode, MVT::i32);
  if(invert)
	  std::swap(LHS, RHS);
	SetCC = DAG.getNode(EpiphanyISD::SETCC, dl, MVT::i32, LHS, RHS, DAG.getCondCode(CC));
  SDValue A64BR_CC = DAG.getNode(EpiphanyISD::BR_CC, dl, MVT::Other, Chain, SetCC, A64cc, DestBB);

  return A64BR_CC;
}

SDValue
EpiphanyTargetLowering::LowerGlobalAddressELF(SDValue Op,
                                             SelectionDAG &DAG) const {
  // TableGen doesn't have easy access to the CodeModel or RelocationModel, so
  // we make that distinction here.

  // We support the small memory model for now.
  assert(getTargetMachine().getCodeModel() == CodeModel::Small);

  EVT PtrVT = getPointerTy();
  DebugLoc dl = Op.getDebugLoc();
  const GlobalAddressSDNode *GN = cast<GlobalAddressSDNode>(Op);
  const GlobalValue *GV = GN->getGlobal();
  unsigned Alignment = GV->getAlignment();
  Reloc::Model RelocM = getTargetMachine().getRelocationModel();

   //DAG.viewGraph();
  //DAG.dump();


  if (GV->isWeakForLinker() && RelocM == Reloc::Static) {
    // Weak symbols can't use ADRP/ADD pair since they should evaluate to
    // zero when undefined. In PIC mode the GOT can take care of this, but in
    // absolute mode we use a constant pool load.
    SDValue PoolAddr;
    PoolAddr = DAG.getNode(EpiphanyISD::WrapperSmall, dl, PtrVT,
                           DAG.getTargetConstantPool(GV, PtrVT, 0, 0, EpiphanyII::MO_HI16),
                           DAG.getTargetConstantPool(GV, PtrVT, 0, 0, EpiphanyII::MO_LO16),
                           DAG.getConstant(4, MVT::i32));
    return DAG.getLoad(PtrVT, dl, DAG.getEntryNode(), PoolAddr,
                       MachinePointerInfo::getConstantPool(),
                       /*isVolatile=*/ false,  /*isNonTemporal=*/ true,
                       /*isInvariant=*/ true, 4);
  }

  if (Alignment == 0) {
    const PointerType *GVPtrTy = cast<PointerType>(GV->getType());
    if (GVPtrTy->getElementType()->isSized()) {
      Alignment = getDataLayout()->getABITypeAlignment(GVPtrTy->getElementType());
    } else {
      // Be conservative if we can't guess, not that it really matters:
      // functions and labels aren't valid for loads, and the methods used to
      // actually calculate an address work with any alignment.
      Alignment = 1;
    }
  }

  SDValue GlobalRef = DAG.getNode(EpiphanyISD::WrapperSmall, dl, PtrVT,
                                  DAG.getTargetGlobalAddress(GV, dl, PtrVT, 0, EpiphanyII::MO_HI16),
                                  DAG.getTargetGlobalAddress(GV, dl, PtrVT, 0, EpiphanyII::MO_LO16),
                                  DAG.getConstant(Alignment, MVT::i32));

  if (GN->getOffset() != 0)
    return DAG.getNode(ISD::ADD, dl, PtrVT, GlobalRef, DAG.getConstant(GN->getOffset(), PtrVT));

  return GlobalRef;
}

SDValue
EpiphanyTargetLowering::LowerJumpTable(SDValue Op, SelectionDAG &DAG) const {
  JumpTableSDNode *JT = cast<JumpTableSDNode>(Op);
  DebugLoc dl = JT->getDebugLoc();

  // When compiling PIC, jump tables get put in the code section so a static
  // relocation-style is acceptable for both cases.
  return DAG.getNode(EpiphanyISD::WrapperSmall, dl, getPointerTy(),
                     DAG.getTargetJumpTable(JT->getIndex(), getPointerTy(), EpiphanyII::MO_HI16),
                     DAG.getTargetJumpTable(JT->getIndex(), getPointerTy(), EpiphanyII::MO_LO16),
                     DAG.getConstant(1, MVT::i32));
}

// (SELECT_CC lhs, rhs, iftrue, iffalse, condcode)
SDValue
EpiphanyTargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const {
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue IfTrue = Op.getOperand(2);
  SDValue IfFalse = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();

  if (LHS.getValueType().isInteger()) {
    SDValue A64cc;

    // Integers are handled in a separate function because the combinations of
    // immediates and tests can get hairy and we may want to fiddle things.
    SDValue CmpOp = getSelectableIntSetCC(LHS, RHS, CC, A64cc, DAG, dl);

    return DAG.getNode(EpiphanyISD::SELECT_CC, dl, Op.getValueType(),
                       CmpOp, IfTrue, IfFalse, A64cc);
  }


  EpiphanyCC::CondCodes CondCode;
  bool invert;
  SDValue SetCC;
  CondCode = FPCCToEpiphanyCC(CC, invert);
  SDValue A64cc = DAG.getConstant(CondCode, MVT::i32);
  if(invert)
	 std::swap(LHS, RHS);
	SetCC = DAG.getNode(EpiphanyISD::SETCC, dl, MVT::i32, LHS, RHS, DAG.getCondCode(CC));
  SDValue A64SELECT_CC = DAG.getNode(EpiphanyISD::SELECT_CC, dl, Op.getValueType(), SetCC, IfTrue, IfFalse, A64cc);

  return A64SELECT_CC;
}

// (SELECT testbit, iftrue, iffalse)
SDValue
EpiphanyTargetLowering::LowerSELECT(SDValue Op, SelectionDAG &DAG) const {
  DebugLoc dl = Op.getDebugLoc();
  SDValue TheBit = Op.getOperand(0);
  SDValue IfTrue = Op.getOperand(1);
  SDValue IfFalse = Op.getOperand(2);

  // Epiphany BooleanContents is the default UndefinedBooleanContent, which means
  // that as the consumer we are responsible for ignoring rubbish in higher
  // bits.
  TheBit = DAG.getNode(ISD::AND, dl, MVT::i32, TheBit,
                       DAG.getConstant(1, MVT::i32));
  SDValue A64CMP = DAG.getNode(EpiphanyISD::SETCC, dl, MVT::i32, TheBit,
                               DAG.getConstant(0, TheBit.getValueType()),
                               DAG.getCondCode(ISD::SETNE));

  return DAG.getNode(EpiphanyISD::SELECT_CC, dl, Op.getValueType(),
                     A64CMP, IfTrue, IfFalse,
                     DAG.getConstant(EpiphanyCC::NE, MVT::i32));
}

// (SETCC lhs, rhs, condcode)
SDValue
EpiphanyTargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  EVT VT = Op.getValueType();

  if (LHS.getValueType().isInteger()) {
    SDValue A64cc;

    // Integers are handled in a separate function because the combinations of
    // immediates and tests can get hairy and we may want to fiddle things.
    SDValue CmpOp = getSelectableIntSetCC(LHS, RHS, CC, A64cc, DAG, dl);

    return DAG.getNode(EpiphanyISD::SELECT_CC, dl, VT,
                       CmpOp, DAG.getConstant(1, VT), DAG.getConstant(0, VT),
                       A64cc);
  }

  EpiphanyCC::CondCodes CondCode;
  bool invert;
  SDValue CmpOp;
  CondCode = FPCCToEpiphanyCC(CC, invert);
  SDValue A64cc = DAG.getConstant(CondCode, MVT::i32);
  if(invert)
	  std::swap(LHS, RHS);
	CmpOp = DAG.getNode(EpiphanyISD::SETCC, dl, MVT::i32, LHS, RHS, DAG.getCondCode(CC));
  SDValue A64SELECT_CC = DAG.getNode(EpiphanyISD::SELECT_CC, dl, VT, CmpOp, DAG.getConstant(1, VT), DAG.getConstant(0, VT), A64cc);

  return A64SELECT_CC;
}

SDValue
EpiphanyTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  default: llvm_unreachable("Don't know how to custom lower this!");

  case ISD::BlockAddress: return LowerBlockAddress(Op, DAG);
  case ISD::BRCOND: return LowerBRCOND(Op, DAG);
  case ISD::BR_CC: return LowerBR_CC(Op, DAG);
  case ISD::FNEG: return LowerFNEG(Op, DAG);
  case ISD::GlobalAddress: return LowerGlobalAddressELF(Op, DAG);
  case ISD::JumpTable: return LowerJumpTable(Op, DAG);
  case ISD::SELECT: return LowerSELECT(Op, DAG);
  case ISD::SELECT_CC: return LowerSELECT_CC(Op, DAG);
  case ISD::SETCC: return LowerSETCC(Op, DAG);
  }

  return SDValue();
}


SDValue
PerformFSUBCombine(SDNode *N, TargetLowering::DAGCombinerInfo &DCI){

SDValue N0 = N->getOperand(0);
SDValue N1 = N->getOperand(1);
EVT VT = N->getValueType(0);
DebugLoc dl = N->getDebugLoc();
SelectionDAG &DAG = DCI.DAG;

	// FSUB -> FMA combines:

    // fold (fsub (fmul x, y), z) -> (fma x, y, (fneg z))
    if (N0.getOpcode() == ISD::FMUL && N0->hasOneUse()) {
		return DAG.getNode(EpiphanyISD::FM_A_S, N->getDebugLoc(), VT, N0.getOperand(0), N0.getOperand(1), N1, DAG.getConstant(1, MVT::i32));
    }

    // fold (fsub x, (fmul y, z)) -> (fma (fneg y), z, x)
    // Note: Commutes FSUB operands.
    if (N1.getOpcode() == ISD::FMUL && N1->hasOneUse()) {
		return DAG.getNode(EpiphanyISD::FM_A_S, N->getDebugLoc(), VT, N1.getOperand(0), N1.getOperand(1), N0, DAG.getConstant(1, MVT::i32));
    }

    // fold (fsub (-(fmul, x, y)), z) -> (fma (fneg x), y, (fneg z))
    //if (N0.getOpcode() == ISD::FNEG && 
    //    N0.getOperand(0).getOpcode() == ISD::FMUL &&
    //    N0->hasOneUse() && N0.getOperand(0).hasOneUse()) {
    //  SDValue N00 = N0.getOperand(0).getOperand(0);
    //  SDValue N01 = N0.getOperand(0).getOperand(1);
    //  return DAG.getNode(ISD::FMA, dl, VT,
    //                     DAG.getNode(ISD::FNEG, dl, VT, N00), N01,
    //                     DAG.getNode(ISD::FNEG, dl, VT, N1));
    //}
  return SDValue();
}

SDValue
PerformFADDCombine(SDNode *N, TargetLowering::DAGCombinerInfo &DCI){

SDValue N0 = N->getOperand(0);
SDValue N1 = N->getOperand(1);
EVT VT = N->getValueType(0);
DebugLoc dl = N->getDebugLoc();
SelectionDAG &DAG = DCI.DAG;

	// FADD -> FMA combines:
	// fold (fadd (fmul x, y), z) -> (fma x, y, z)
	if (N0.getOpcode() == ISD::FMUL && N0->hasOneUse()) {
		return DAG.getNode(EpiphanyISD::FM_A_S, N->getDebugLoc(), VT, N0.getOperand(0), N0.getOperand(1), N1, DAG.getConstant(0, MVT::i32));
	}

	// fold (fadd x, (fmul y, z)) -> (fma y, z, x)
	// Note: Commutes FADD operands.
	if (N1.getOpcode() == ISD::FMUL && N1->hasOneUse()) {
		return DAG.getNode(EpiphanyISD::FM_A_S, N->getDebugLoc(), VT, N1.getOperand(0), N1.getOperand(1), N0, DAG.getConstant(0, MVT::i32));
	}

	return SDValue();
}

SDValue
EpiphanyTargetLowering::PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const {
		switch (N->getOpcode()) {
		case ISD::FADD: return PerformFADDCombine(N, DCI);
		case ISD::FSUB: return PerformFSUBCombine(N, DCI);
		default: break;
		}
		return SDValue();
}

