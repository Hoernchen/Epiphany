//==-- EpiphanyISelLowering.h - Epiphany DAG Lowering Interface ----*- C++ -*-==//
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

#ifndef LLVM_TARGET_EPIPHANY_ISELLOWERING_H
#define LLVM_TARGET_EPIPHANY_ISELLOWERING_H

#include "Utils/EpiphanyBaseInfo.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Target/TargetLowering.h"


namespace llvm {
namespace EpiphanyISD {
  enum NodeType {
    // Start the numbering from where ISD NodeType finishes.
    FIRST_NUMBER = ISD::BUILTIN_OP_END,

    // This is a conditional branch which also notes the flag needed
    // (eq/sgt/...). A64 puts this information on the branches rather than
    // compares as LLVM does.
    BR_CC,

    // A node to be selected to an actual call operation: either BL or BLR in
    // the absence of tail calls.
    Call,

    // Simply a convenient node inserted during ISelLowering to represent
    // procedure return. Will almost certainly be selected to "RET".
    Ret,

    /// This is an A64-ification of the standard LLVM SELECT_CC operation. The
    /// main difference is that it only has the values and an A64 condition,
    /// which will be produced by a setcc instruction.
    SELECT_CC,

    /// This serves most of the functions of the LLVM SETCC instruction, for two
    /// purposes. First, it prevents optimisations from fiddling with the
    /// compare after we've moved the CondCode information onto the SELECT_CC or
    /// BR_CC instructions. Second, it gives a legal instruction for the actual
    /// comparison.
    ///
    /// It keeps a record of the condition flags asked for because certain
    /// instructions are only valid for a subset of condition codes.
    SETCC,

    // Wraps an address which the ISelLowering phase has decided should be
    // created using the small absolute memory model: i.e. adrp/add or
    // adrp/mem-op. This exists to prevent bare TargetAddresses which may never
    // get selected.
	WrapperSmall,

	// Node for FMA and FMS
	FM_A_S
  };
}


class EpiphanySubtarget;
class EpiphanyTargetMachine;

class EpiphanyTargetLowering : public TargetLowering {
public:
  explicit EpiphanyTargetLowering(EpiphanyTargetMachine &TM);

  const char *getTargetNodeName(unsigned Opcode) const;

  CCAssignFn *CCAssignFnForNode(CallingConv::ID CC) const;

  SDValue LowerFormalArguments(SDValue Chain,
                               CallingConv::ID CallConv, bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               DebugLoc dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const;

  SDValue LowerReturn(SDValue Chain,
                      CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals,
                      DebugLoc dl, SelectionDAG &DAG) const;

  SDValue LowerCall(CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const;

  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          CallingConv::ID CallConv, bool IsVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          DebugLoc dl, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;

  void SaveVarArgRegisters(CCState &CCInfo, SelectionDAG &DAG,
                           DebugLoc DL, SDValue &Chain) const;


  /// Finds the incoming stack arguments which overlap the given fixed stack
  /// object and incorporates their load into the current chain. This prevents
  /// an upcoming store from clobbering the stack argument before it's used.
  SDValue addTokenForArgument(SDValue Chain, SelectionDAG &DAG,
                              MachineFrameInfo *MFI, int ClobberedFI) const;

  EVT getSetCCResultType(EVT VT) const;

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;

  bool isLegalICmpImmediate(int64_t Val) const;
  SDValue getSelectableIntSetCC(SDValue LHS, SDValue RHS, ISD::CondCode CC,
                         SDValue &A64cc, SelectionDAG &DAG, DebugLoc &dl) const;

  virtual MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr *MI, MachineBasicBlock *MBB) const;

  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBRCOND(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFNEG(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddressELF(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;

  virtual SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const;

  //nope.
  bool IsEligibleForTailCallOptimization(SDValue Callee,
                                    CallingConv::ID CalleeCC,
                                    bool IsVarArg,
                                    bool IsCalleeStructRet,
                                    bool IsCallerStructRet,
                                    const SmallVectorImpl<ISD::OutputArg> &Outs,
                                    const SmallVectorImpl<SDValue> &OutVals,
                                    const SmallVectorImpl<ISD::InputArg> &Ins,
									SelectionDAG& DAG) const { return false;}
// custom fma due to fneg, so we say no
  virtual bool isFMAFasterThanMulAndAdd(EVT) const { return false; }

private:
  const EpiphanySubtarget *Subtarget;
  const TargetRegisterInfo *RegInfo;
  const InstrItineraryData *Itins;
};
} // namespace llvm

#endif // LLVM_TARGET_EPIPHANY_ISELLOWERING_H
