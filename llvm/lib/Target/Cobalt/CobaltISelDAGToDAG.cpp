//===-- CobaltISelDAGToDAG.cpp - Cobalt DAG selector -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Cobalt.h"
#include "CobaltISelLowering.h"
#include "CobaltTargetMachine.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "cobalt-isel"
#define PASS_NAME "Cobalt DAG->DAG Instruction Selection"

namespace {
constexpr unsigned WorkgroupAddressSpace = 3;

static uint64_t getFrameObjectByteOffset(MachineFunction &MF, int FrameIndex) {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t Offset = 0;

  for (int I = MFI.getObjectIndexBegin(), E = MFI.getObjectIndexEnd(); I != E;
       ++I) {
    if (I < 0 || MFI.isDeadObjectIndex(I))
      continue;

    const int64_t Size = MFI.getObjectSize(I);
    if (Size <= 0)
      report_fatal_error(
          "Cobalt shared/private frame objects must have fixed size");

    Offset = alignTo(Offset, MFI.getObjectAlign(I));
    if (I == FrameIndex)
      return Offset;
    Offset += static_cast<uint64_t>(Size);
  }

  report_fatal_error("Cobalt frame index was not in MachineFrameInfo");
}

class CobaltDAGToDAGISel : public SelectionDAGISel {
public:
  CobaltDAGToDAGISel() = delete;

  CobaltDAGToDAGISel(CobaltTargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

private:
#include "CobaltGenDAGISel.inc"

  SDValue materializeI32(uint32_t Raw, const SDLoc &DL);
  SDValue materializeF32(uint32_t Raw, const SDLoc &DL);
  void Select(SDNode *N) override;
};

class CobaltDAGToDAGISelLegacy : public SelectionDAGISelLegacy {
public:
  static char ID;
  CobaltDAGToDAGISelLegacy(CobaltTargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISelLegacy(
            ID, std::make_unique<CobaltDAGToDAGISel>(TM, OptLevel)) {}
};
} // namespace

char CobaltDAGToDAGISelLegacy::ID = 0;

INITIALIZE_PASS(CobaltDAGToDAGISelLegacy, DEBUG_TYPE, PASS_NAME, false, false)

SDValue CobaltDAGToDAGISel::materializeI32(uint32_t Raw, const SDLoc &DL) {
  const uint32_t Lo = Raw & 0xffffu;
  const uint32_t Hi = (Raw >> 16) & 0xffffu;

  SDValue LoImm = CurDAG->getTargetConstant(Lo, DL, MVT::i32);
  SDNode *LoNode = CurDAG->getMachineNode(Cobalt::VMOVI, DL, MVT::i32, LoImm);
  if (Hi == 0)
    return SDValue(LoNode, 0);

  SDValue HiImm = CurDAG->getTargetConstant(Hi, DL, MVT::i32);
  SDNode *HiNode = CurDAG->getMachineNode(Cobalt::VMOVHI, DL, MVT::i32,
                                          SDValue(LoNode, 0), HiImm);
  return SDValue(HiNode, 0);
}

SDValue CobaltDAGToDAGISel::materializeF32(uint32_t Raw, const SDLoc &DL) {
  const uint32_t Lo = Raw & 0xffffu;
  const uint32_t Hi = (Raw >> 16) & 0xffffu;

  SDValue LoImm = CurDAG->getTargetConstant(Lo, DL, MVT::i32);
  SDNode *LoNode =
      CurDAG->getMachineNode(Cobalt::VMOVIF, DL, MVT::f32, LoImm);
  if (Hi == 0)
    return SDValue(LoNode, 0);

  SDValue HiImm = CurDAG->getTargetConstant(Hi, DL, MVT::i32);
  SDNode *HiNode = CurDAG->getMachineNode(Cobalt::VMOVHIF, DL, MVT::f32,
                                          SDValue(LoNode, 0), HiImm);
  return SDValue(HiNode, 0);
}

void CobaltDAGToDAGISel::Select(SDNode *N) {
  if (N->isMachineOpcode()) {
    LLVM_DEBUG(errs() << "== "; N->dump(CurDAG); errs() << "\n");
    N->setNodeId(-1);
    return;
  }

  const SDLoc DL(N);
  switch (N->getOpcode()) {
  case CobaltISD::BRCOND:
    CurDAG->SelectNodeTo(N, Cobalt::BRCOND, MVT::Other, N->getOperand(1),
                         N->getOperand(2), N->getOperand(0));
    return;
  case ISD::LOAD: {
    auto *Ld = cast<LoadSDNode>(N);
    if (Ld->getAddressSpace() == WorkgroupAddressSpace &&
        Ld->getMemoryVT() == MVT::i32) {
      CurDAG->SelectNodeTo(N, Cobalt::VLDS, MVT::i32, MVT::Other,
                           N->getOperand(1), N->getOperand(0));
      return;
    }
    break;
  }
  case ISD::STORE: {
    auto *St = cast<StoreSDNode>(N);
    if (St->getAddressSpace() == WorkgroupAddressSpace &&
        St->getMemoryVT() == MVT::i32) {
      CurDAG->SelectNodeTo(N, Cobalt::VSTS, MVT::Other, N->getOperand(1),
                           N->getOperand(2), N->getOperand(0));
      return;
    }
    if (St->getMemoryVT() == MVT::f32) {
      auto *CFP = dyn_cast<ConstantFPSDNode>(N->getOperand(1));
      if (CFP) {
        const APInt Bits = CFP->getValueAPF().bitcastToAPInt();
        const uint32_t Raw = static_cast<uint32_t>(Bits.getZExtValue());
        SDValue Src = materializeF32(Raw, DL);
        CurDAG->SelectNodeTo(N, Cobalt::VSTF, MVT::Other, Src,
                             N->getOperand(2), N->getOperand(0));
        return;
      }
    }
    if (St->getMemoryVT() == MVT::i32) {
      auto *C = dyn_cast<ConstantSDNode>(N->getOperand(1));
      if (C) {
        const uint32_t Raw =
            static_cast<uint32_t>(C->getAPIntValue().getZExtValue());
        SDValue Src = materializeI32(Raw, DL);
        CurDAG->SelectNodeTo(N, Cobalt::VST, MVT::Other, Src,
                             N->getOperand(2), N->getOperand(0));
        return;
      }
    }
    break;
  }
  case ISD::ATOMIC_LOAD_ADD: {
    auto *Atomic = cast<AtomicSDNode>(N);
    if (Atomic->getMemoryVT() != MVT::i32)
      break;

    const unsigned Opc = Atomic->getAddressSpace() == WorkgroupAddressSpace
                             ? Cobalt::VATOMIADDS
                             : Cobalt::VATOMIADD;
    SDValue Ops[] = {N->getOperand(1), N->getOperand(2), N->getOperand(0)};
    CurDAG->SelectNodeTo(N, Opc, CurDAG->getVTList(MVT::i32, MVT::Other),
                         Ops);
    return;
  }
  case ISD::ConstantFP: {
    auto *CFP = cast<ConstantFPSDNode>(N);
    if (N->getValueType(0) != MVT::f32)
      break;

    const APInt Bits = CFP->getValueAPF().bitcastToAPInt();
    const uint32_t Raw = static_cast<uint32_t>(Bits.getZExtValue());
    ReplaceNode(N, materializeF32(Raw, DL).getNode());
    return;
  }
  case ISD::FrameIndex: {
    const int FI = cast<FrameIndexSDNode>(N)->getIndex();
    const uint64_t Offset =
        getFrameObjectByteOffset(CurDAG->getMachineFunction(), FI);
    if (Offset > 0xffff)
      report_fatal_error("Cobalt frame object offset exceeds VMOVI imm16");

    SDValue OffsetImm =
        CurDAG->getTargetConstant(static_cast<unsigned>(Offset), DL, MVT::i32);
    CurDAG->SelectNodeTo(N, Cobalt::VMOVI, MVT::i32, OffsetImm);
    return;
  }
  case ISD::SHL: {
    // CobaltISA 1.0 has no shift opcode. Select constant left shifts directly
    // as VMOVI + VMUL here instead of lowering them to ISD::MUL earlier:
    // SelectionDAG combines multiply-by-power-of-two back into SHL.
    auto *Amount = dyn_cast<ConstantSDNode>(N->getOperand(1));
    if (!Amount)
      break;

    uint64_t Shift = Amount->getZExtValue();
    if (Shift == 0) {
      ReplaceNode(N, N->getOperand(0).getNode());
      return;
    }
    if (Shift < 15) {
      SDValue ScaleImm = CurDAG->getTargetConstant(1u << Shift, DL, MVT::i32);
      SDNode *Scale =
          CurDAG->getMachineNode(Cobalt::VMOVI, DL, MVT::i32, ScaleImm);
      CurDAG->SelectNodeTo(N, Cobalt::VMUL, MVT::i32, N->getOperand(0),
                           SDValue(Scale, 0));
      return;
    }
    report_fatal_error(
        "Cobalt only supports constant i32 left shifts by 0..14 bits");
  }
  default:
    break;
  }

  SelectCode(N);
}

FunctionPass *llvm::createCobaltISelDag(CobaltTargetMachine &TM,
                                        CodeGenOptLevel OptLevel) {
  return new CobaltDAGToDAGISelLegacy(TM, OptLevel);
}
