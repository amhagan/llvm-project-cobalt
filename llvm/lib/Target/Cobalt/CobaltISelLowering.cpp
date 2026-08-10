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
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/ErrorHandling.h"
#include <optional>

using namespace llvm;

static const MCPhysReg ArgRegs[] = {
    Cobalt::R0,  Cobalt::R1,  Cobalt::R2,  Cobalt::R3,  Cobalt::R4,
    Cobalt::R5,  Cobalt::R6,  Cobalt::R7,  Cobalt::R8,  Cobalt::R9,
    Cobalt::R10, Cobalt::R11, Cobalt::R12, Cobalt::R13,
};

static bool IsLaunchSgprSlot(unsigned AbiSlot) {
  switch (AbiSlot) {
  case 0: // gid_linear wave base
  case 1: // gid_x wave base
  case 4: // array_len16_per_workitem
  case 5: // dispatch_grid_z
    return true;
  default:
    return false;
  }
}

static std::optional<unsigned> GetNamedLaunchSgprSlot(StringRef Name) {
  return StringSwitch<std::optional<unsigned>>(Name)
      .Case("gid_linear", 0)
      .Case("gid_x", 1)
      .Case("num_workgroups_x", 2)
      .Case("num_workgroups_y", 3)
      .Case("array_len16", 4)
      .Case("array_len16_per_workitem", 4)
      .Case("dispatch_grid_z", 5)
      .Case("num_workgroups_z", 6)
      .Default(std::nullopt);
}

static std::optional<unsigned> GetUserDataSource(StringRef Name) {
  return StringSwitch<std::optional<unsigned>>(Name)
      .Case("desc0_base_lo", 0)
      .Case("desc1_base_lo", 1)
      .Case("desc2_base_lo", 2)
      .Case("desc3_base_lo", 3)
      .Case("desc0_size_bytes", 4)
      .Case("desc1_size_bytes", 5)
      .Case("desc2_size_bytes", 6)
      .Case("desc3_size_bytes", 7)
      .Case("uniform_b0_word0", 8)
      .Case("uniform_b1_word0", 9)
      .Case("uniform_b2_word0", 10)
      .Case("uniform_b3_word0", 11)
      .Case("uniform_b0_word1", 12)
      .Case("uniform_b1_word1", 13)
      .Case("uniform_b2_word1", 14)
      .Case("uniform_b3_word1", 15)
      .Case("uniform_b0_word2", 16)
      .Case("uniform_b1_word2", 17)
      .Case("uniform_b2_word2", 18)
      .Case("uniform_b3_word2", 19)
      .Case("uniform_b0_word3", 20)
      .Case("uniform_b1_word3", 21)
      .Case("uniform_b2_word3", 22)
      .Case("uniform_b3_word3", 23)
      .Case("push_constant_word0", 24)
      .Case("push_constant_word1", 25)
      .Case("push_constant_word2", 26)
      .Case("push_constant_word3", 27)
      .Case("push_constant_word4", 28)
      .Case("push_constant_word5", 29)
      .Case("push_constant_word6", 30)
      .Case("push_constant_word7", 31)
      .Default(std::nullopt);
}

static std::optional<unsigned>
GetPackedUserDataSgprSlot(const Function &F, StringRef Name) {
  std::optional<unsigned> TargetSource = GetUserDataSource(Name);
  if (!TargetSource)
    return std::nullopt;

  Attribute MapAttr = F.getFnAttribute("cobalt-user-data-map");
  if (MapAttr.isValid()) {
    SmallVector<StringRef, 8> Sources;
    MapAttr.getValueAsString().split(Sources, ',', -1, false);
    if (Sources.size() > 8)
      report_fatal_error("Cobalt user-data map exceeds s8..s15");
    for (unsigned I = 0; I < Sources.size(); ++I) {
      unsigned Source = 0;
      if (Sources[I].getAsInteger(0, Source) || Source > 31)
        report_fatal_error("invalid Cobalt user-data source map");
      if (Source == *TargetSource)
        return 8u + I;
    }
    return std::nullopt;
  }

  // Standalone llc input has no pipeline metadata. Recreate the canonical
  // descriptor-first packing from the live named formals.
  unsigned PackedIndex = 0;
  for (unsigned Source = 0; Source <= 31; ++Source) {
    const Argument *SourceArg = nullptr;
    for (const Argument &Arg : F.args()) {
      if (GetUserDataSource(Arg.getName()) == Source) {
        SourceArg = &Arg;
        break;
      }
    }
    if (!SourceArg || SourceArg->use_empty())
      continue;
    if (PackedIndex >= 8)
      report_fatal_error("Cobalt user-data inputs exceed s8..s15");
    if (Source == *TargetSource)
      return 8u + PackedIndex;
    ++PackedIndex;
  }
  return std::nullopt;
}

static bool IsUserDataSgprSlot(unsigned SgprSlot) {
  return SgprSlot >= 8 && SgprSlot <= 15;
}

static bool getVSplatSgprIndex(SDValue Value, unsigned &Sgpr) {
  if (!Value.isMachineOpcode() ||
      Value.getMachineOpcode() != Cobalt::VSPLATSGPR)
    return false;

  auto *SgprNode = dyn_cast<ConstantSDNode>(Value.getOperand(0));
  if (!SgprNode)
    return false;

  Sgpr = static_cast<unsigned>(SgprNode->getZExtValue());
  return true;
}

static SDValue makeSgprOperandALU(unsigned MachineOpcode, EVT ResultVT,
                                  SDValue Vector, unsigned Sgpr, SDNode *N,
                                  SelectionDAG &DAG) {
  const SDLoc DL(N);
  return SDValue(
      DAG.getMachineNode(MachineOpcode, DL, ResultVT, Vector,
                         DAG.getTargetConstant(Sgpr, DL, MVT::i32)),
      0);
}

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
  setOperationAction(ISD::SETCC, MVT::f32, Legal);
  setOperationAction(ISD::SELECT, MVT::f32, Legal);
  setOperationAction(ISD::ConstantFP, MVT::f32, Legal);
  setOperationAction(ISD::FADD, MVT::f32, Legal);
  setOperationAction(ISD::FSUB, MVT::f32, Legal);
  setOperationAction(ISD::FNEG, MVT::f32, Legal);
  setOperationAction(ISD::FABS, MVT::f32, Legal);
  setOperationAction(ISD::FMUL, MVT::f32, Legal);
  setOperationAction(ISD::SINT_TO_FP, MVT::f32, Legal);
  setOperationAction(ISD::UINT_TO_FP, MVT::f32, Legal);
  setOperationAction(ISD::FP_TO_SINT, MVT::i32, Legal);
  setOperationAction(ISD::FP_TO_UINT, MVT::i32, Legal);
  setOperationAction(ISD::UDIV, MVT::i32, Legal);
  setOperationAction(ISD::UREM, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_ADD, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_SWAP, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_CMP_SWAP, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_AND, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_OR, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_XOR, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_MIN, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_MAX, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_UMIN, MVT::i32, Legal);
  setOperationAction(ISD::ATOMIC_LOAD_UMAX, MVT::i32, Legal);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_CC, MVT::f32, Custom);
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Legal);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);

  setTargetDAGCombine(ISD::ADD);
  setTargetDAGCombine(ISD::SUB);
  setTargetDAGCombine(ISD::MUL);
  setTargetDAGCombine(ISD::AND);
  setTargetDAGCombine(ISD::OR);
  setTargetDAGCombine(ISD::XOR);
  setTargetDAGCombine(ISD::FADD);
  setTargetDAGCombine(ISD::FSUB);
  setTargetDAGCombine(ISD::FMUL);
  setTargetDAGCombine(ISD::SETCC);
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
  const bool UseLaunchSgprAbi =
      MF.getFunction().hasFnAttribute("cobalt-launch-sgpr-abi");

  for (unsigned I = 0, E = Ins.size(); I != E; ++I) {
    // Cobalt compute kernels receive buffer pointers as symbolic binding
    // handles. The SIMD hardware gets real buffer bases from the descriptor
    // table selected by VLD/VST imm bits, while scalar launch coordinates are
    // supplied by the CP launch ABI. Keep r14/r15 reserved for the hardware
    // lane id and compiler zero register conventions.
    if (Ins[I].OrigTy && Ins[I].OrigTy->isPointerTy()) {
      InVals.push_back(DAG.getConstant(0, DL, MVT::i32));
      continue;
    }

    std::optional<unsigned> NamedSgpr;
    bool IsUserDataArg = false;
    if (UseLaunchSgprAbi && I < MF.getFunction().arg_size()) {
      const Argument *Arg = MF.getFunction().getArg(I);
      NamedSgpr = GetNamedLaunchSgprSlot(Arg->getName());
      IsUserDataArg = GetUserDataSource(Arg->getName()).has_value();
      if (!NamedSgpr && IsUserDataArg)
        NamedSgpr =
            GetPackedUserDataSgprSlot(MF.getFunction(), Arg->getName());
    }
    if (Ins[I].VT != MVT::i32 &&
        !(Ins[I].VT == MVT::f32 && IsUserDataArg))
      report_fatal_error(
          "Cobalt only supports i32 arguments and packed f32 user data");

    // Unused user-data formals remain in normalized LLVM signatures but do
    // not occupy either packed SGPRs or positional VGPR argument slots.
    if (IsUserDataArg && !NamedSgpr) {
      InVals.push_back(DAG.getConstant(0, DL, MVT::i32));
      continue;
    }

    // Dense launch ABI formals are still positional, even when lowered through
    // direct SGPR operands. Descriptor and uniform user-data pseudo-args are
    // separate compiler-visible operands and do not consume r0..r13 launch
    // positions.
    const bool ConsumesScalarArg =
        !NamedSgpr || !IsUserDataSgprSlot(*NamedSgpr);
    unsigned AbiSlot = 0;
    if (ConsumesScalarArg) {
      AbiSlot = ScalarArg++;
      if (!NamedSgpr && AbiSlot >= std::size(ArgRegs))
        report_fatal_error("Cobalt supports at most fourteen scalar arguments");
    }

    if (NamedSgpr) {
      const unsigned SgprSlot = *NamedSgpr;
      if (SgprSlot == 0 || SgprSlot == 1) {
        Register LaneVReg = MRI.createVirtualRegister(&Cobalt::VGPR32RegClass);
        MRI.addLiveIn(Cobalt::R14, LaneVReg);
        SDValue Lane = DAG.getCopyFromReg(Chain, DL, LaneVReg, MVT::i32);
        InVals.push_back(SDValue(
            DAG.getMachineNode(Cobalt::VADDSGPR, DL, MVT::i32, Lane,
                               DAG.getTargetConstant(SgprSlot, DL, MVT::i32)),
            0));
      } else {
        InVals.push_back(SDValue(
            DAG.getMachineNode(Cobalt::VSPLATSGPR, DL, Ins[I].VT,
                               DAG.getTargetConstant(SgprSlot, DL, MVT::i32)),
            0));
      }
      continue;
    }

    if (UseLaunchSgprAbi && IsLaunchSgprSlot(AbiSlot)) {
      if (AbiSlot == 0 || AbiSlot == 1) {
        Register LaneVReg = MRI.createVirtualRegister(&Cobalt::VGPR32RegClass);
        MRI.addLiveIn(Cobalt::R14, LaneVReg);
        SDValue Lane = DAG.getCopyFromReg(Chain, DL, LaneVReg, MVT::i32);
        InVals.push_back(SDValue(
            DAG.getMachineNode(Cobalt::VADDSGPR, DL, MVT::i32, Lane,
                               DAG.getTargetConstant(AbiSlot, DL, MVT::i32)),
            0));
      } else {
        SDValue SgprValue = SDValue(
            DAG.getMachineNode(Cobalt::VSPLATSGPR, DL, MVT::i32,
                               DAG.getTargetConstant(AbiSlot, DL, MVT::i32)),
            0);
        InVals.push_back(SgprValue);
      }
      continue;
    }

    Register VReg = MRI.createVirtualRegister(&Cobalt::VGPR32RegClass);
    MRI.addLiveIn(ArgRegs[AbiSlot], VReg);
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
    if (Op.getValueType() == MVT::f32)
      return DAG.getNode(ISD::SELECT, DL, MVT::f32, Cond, TrueVal, FalseVal);
    SDValue Diff = DAG.getNode(ISD::SUB, DL, MVT::i32, TrueVal, FalseVal);
    SDValue Scaled = DAG.getNode(ISD::MUL, DL, MVT::i32, Cond, Diff);
    return DAG.getNode(ISD::ADD, DL, MVT::i32, FalseVal, Scaled);
  }
  default:
    report_fatal_error("unexpected Cobalt custom lowering opcode");
  }
}

SDValue CobaltTargetLowering::PerformDAGCombine(SDNode *N,
                                                DAGCombinerInfo &DCI) const {
  if (N->getValueType(0) != MVT::i32 &&
      N->getValueType(0) != MVT::f32)
    return SDValue();

  SelectionDAG &DAG = DCI.DAG;
  SDValue LHS = N->getOperand(0);
  SDValue RHS = N->getOperand(1);
  unsigned Sgpr = 0;

  switch (N->getOpcode()) {
  case ISD::ADD:
    if (getVSplatSgprIndex(RHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VADDSGPR, MVT::i32, LHS, Sgpr, N,
                                DAG);
    if (getVSplatSgprIndex(LHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VADDSGPR, MVT::i32, RHS, Sgpr, N,
                                DAG);
    break;
  case ISD::SUB:
    if (getVSplatSgprIndex(RHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VSUBSGPR, MVT::i32, LHS, Sgpr, N,
                                DAG);
    break;
  case ISD::MUL:
    if (getVSplatSgprIndex(RHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VMULSGPR, MVT::i32, LHS, Sgpr, N,
                                DAG);
    if (getVSplatSgprIndex(LHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VMULSGPR, MVT::i32, RHS, Sgpr, N,
                                DAG);
    break;
  case ISD::AND:
    if (getVSplatSgprIndex(RHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VANDSGPR, MVT::i32, LHS, Sgpr, N,
                                DAG);
    if (getVSplatSgprIndex(LHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VANDSGPR, MVT::i32, RHS, Sgpr, N,
                                DAG);
    break;
  case ISD::OR:
    if (getVSplatSgprIndex(RHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VORSGPR, MVT::i32, LHS, Sgpr, N,
                                DAG);
    if (getVSplatSgprIndex(LHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VORSGPR, MVT::i32, RHS, Sgpr, N,
                                DAG);
    break;
  case ISD::XOR:
    if (getVSplatSgprIndex(RHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VXORSGPR, MVT::i32, LHS, Sgpr, N,
                                DAG);
    if (getVSplatSgprIndex(LHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VXORSGPR, MVT::i32, RHS, Sgpr, N,
                                DAG);
    break;
  case ISD::FADD:
    if (getVSplatSgprIndex(RHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VFADDSGPR, MVT::f32, LHS, Sgpr, N,
                                DAG);
    if (getVSplatSgprIndex(LHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VFADDSGPR, MVT::f32, RHS, Sgpr, N,
                                DAG);
    break;
  case ISD::FMUL:
    if (getVSplatSgprIndex(RHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VFMULSGPR, MVT::f32, LHS, Sgpr, N,
                                DAG);
    if (getVSplatSgprIndex(LHS, Sgpr))
      return makeSgprOperandALU(Cobalt::VFMULSGPR, MVT::f32, RHS, Sgpr, N,
                                DAG);
    break;
  case ISD::SETCC:
    if (getVSplatSgprIndex(RHS, Sgpr)) {
      const auto CC = cast<CondCodeSDNode>(N->getOperand(2))->get();
      const bool IsFP = LHS.getValueType() == MVT::f32;
      switch (CC) {
      case ISD::SETULT:
        if (!IsFP)
          return makeSgprOperandALU(Cobalt::VCMPULTSGPR, MVT::i32, LHS, Sgpr,
                                    N, DAG);
        break;
      case ISD::SETULE:
        if (!IsFP)
          return makeSgprOperandALU(Cobalt::VCMPULESGPR, MVT::i32, LHS, Sgpr,
                                    N, DAG);
        break;
      case ISD::SETUGE:
        if (!IsFP)
          return makeSgprOperandALU(Cobalt::VCMPUGESGPR, MVT::i32, LHS, Sgpr,
                                    N, DAG);
        break;
      case ISD::SETOEQ:
        if (IsFP)
          return makeSgprOperandALU(Cobalt::VFCMPEQSGPR, MVT::i32, LHS, Sgpr,
                                    N, DAG);
        break;
      case ISD::SETUNE:
        if (IsFP)
          return makeSgprOperandALU(Cobalt::VFCMPNESGPR, MVT::i32, LHS, Sgpr,
                                    N, DAG);
        break;
      case ISD::SETOLT:
        if (IsFP)
          return makeSgprOperandALU(Cobalt::VFCMPLTSGPR, MVT::i32, LHS, Sgpr,
                                    N, DAG);
        break;
      case ISD::SETOLE:
        if (IsFP)
          return makeSgprOperandALU(Cobalt::VFCMPLESGPR, MVT::i32, LHS, Sgpr,
                                    N, DAG);
        break;
      default:
        break;
      }
    }
    break;
  default:
    break;
  }

  return SDValue();
}
