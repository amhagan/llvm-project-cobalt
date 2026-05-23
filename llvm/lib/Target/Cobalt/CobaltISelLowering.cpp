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
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static const MCPhysReg ArgRegs[] = {Cobalt::R0, Cobalt::R1, Cobalt::R2,
                                    Cobalt::R3};

CobaltTargetLowering::CobaltTargetLowering(const TargetMachine &TM,
                                           const CobaltSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i32, &Cobalt::VGPR32RegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  setStackPointerRegisterToSaveRestore(Cobalt::R0);
  setMinFunctionAlignment(Align(4));
  setPrefFunctionAlignment(Align(4));
}

SDValue CobaltTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (IsVarArg)
    report_fatal_error("Cobalt argument lowering is not implemented yet");
  if (Ins.size() > std::size(ArgRegs))
    report_fatal_error("Cobalt supports at most four smoke-test arguments");

  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  for (unsigned I = 0, E = Ins.size(); I != E; ++I) {
    if (Ins[I].VT != MVT::i32)
      report_fatal_error("Cobalt only supports i32 smoke-test arguments");

    Register VReg = MRI.createVirtualRegister(&Cobalt::VGPR32RegClass);
    MRI.addLiveIn(ArgRegs[I], VReg);
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
