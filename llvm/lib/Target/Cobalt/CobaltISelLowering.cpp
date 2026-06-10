//===-- CobaltISelLowering.cpp - Cobalt DAG lowering ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltISelLowering.h"
#include "CobaltSubtarget.h"
#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static const MCPhysReg ArgRegs[] = {
    Cobalt::R0,  Cobalt::R1,  Cobalt::R2,  Cobalt::R3,  Cobalt::R4,
    Cobalt::R5,  Cobalt::R6,  Cobalt::R7,  Cobalt::R8,  Cobalt::R9,
    Cobalt::R10, Cobalt::R11, Cobalt::R12, Cobalt::R13,
};

CobaltTargetLowering::CobaltTargetLowering(const TargetMachine &TM,
                                           const CobaltSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i32, &Cobalt::VGPR32RegClass);
  addRegisterClass(MVT::f32, &Cobalt::FGPR32RegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  setStackPointerRegisterToSaveRestore(Cobalt::R0);
  setMinFunctionAlignment(Align(4));
  setPrefFunctionAlignment(Align(4));

  setOperationAction(ISD::SETCC, MVT::i32, Legal);
  setOperationAction(ISD::ConstantFP, MVT::f32, Legal);
  setOperationAction(ISD::FADD, MVT::f32, Legal);
  setOperationAction(ISD::FMUL, MVT::f32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_ADD, MVT::i32, Legal);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Legal);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
}

SDValue CobaltTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (IsVarArg)
    report_fatal_error("Cobalt argument lowering is not implemented yet");
  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  unsigned ScalarArg = 0;

  for (unsigned I = 0, E = Ins.size(); I != E; ++I) {
    if (Ins[I].VT != MVT::i32)
      report_fatal_error("Cobalt only supports i32 smoke-test arguments");

    // Cobalt compute kernels receive buffer pointers as symbolic binding
    // handles. The SIMD hardware gets real buffer bases from the descriptor
    // table selected by VLD/VST imm bits, while scalar launch coordinates are
    // supplied by the CP launch ABI. Keep r14/r15 reserved for the hardware
    // lane id and compiler zero register conventions.
    if (Ins[I].OrigTy && Ins[I].OrigTy->isPointerTy()) {
      InVals.push_back(DAG.getConstant(0, DL, MVT::i32));
      continue;
    }

    if (ScalarArg >= std::size(ArgRegs))
      report_fatal_error("Cobalt supports at most fourteen scalar arguments");

    Register VReg = MRI.createVirtualRegister(&Cobalt::VGPR32RegClass);
    MRI.addLiveIn(ArgRegs[ScalarArg++], VReg);
    InVals.push_back(DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32));
  }

  return Chain;
}

bool CobaltTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  return !IsVarArg && Outs.size() <= 1 &&
         (Outs.empty() || Outs[0].VT == MVT::i32);
}

SDValue
CobaltTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                  bool IsVarArg,
                                  const SmallVectorImpl<ISD::OutputArg> &Outs,
                                  const SmallVectorImpl<SDValue> &OutVals,
                                  const SDLoc &DL, SelectionDAG &DAG) const {
  if (IsVarArg || Outs.size() > 1)
    report_fatal_error("Cobalt return-value lowering is not implemented yet");
  if (Outs.empty())
    return DAG.getNode(CobaltISD::RET, DL, MVT::Other, Chain);

  if (Outs[0].VT != MVT::i32)
    report_fatal_error("Cobalt only supports i32 smoke-test returns");

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  Chain = DAG.getCopyToReg(Chain, DL, Cobalt::R0, OutVals[0], Glue);
  Glue = Chain.getValue(1);

  RetOps[0] = Chain;
  RetOps.push_back(DAG.getRegister(Cobalt::R0, MVT::i32));
  RetOps.push_back(Glue);
  return DAG.getNode(CobaltISD::RET, DL, MVT::Other, RetOps);
}

SDValue CobaltTargetLowering::LowerOperation(SDValue Op,
                                             SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::BR_CC: {
    SDLoc DL(Op);
    auto *CC = cast<CondCodeSDNode>(Op.getOperand(1));
    SDValue Cond = DAG.getSetCC(DL, MVT::i32, Op.getOperand(2),
                                Op.getOperand(3), CC->get());
    return DAG.getNode(CobaltISD::BRCOND, DL, MVT::Other, Op.getOperand(0),
                       Cond, Op.getOperand(4));
  }
  case ISD::SELECT_CC: {
    SDLoc DL(Op);
    auto *CC = cast<CondCodeSDNode>(Op.getOperand(4));
    SDValue Cond = DAG.getSetCC(DL, MVT::i32, Op.getOperand(0),
                                Op.getOperand(1), CC->get());
    SDValue TrueVal = Op.getOperand(2);
    SDValue FalseVal = Op.getOperand(3);
    SDValue Diff = DAG.getNode(ISD::SUB, DL, MVT::i32, TrueVal, FalseVal);
    SDValue Scaled = DAG.getNode(ISD::MUL, DL, MVT::i32, Cond, Diff);
    return DAG.getNode(ISD::ADD, DL, MVT::i32, FalseVal, Scaled);
  }
  default:
    report_fatal_error("unexpected Cobalt custom lowering opcode");
  }
}
