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
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "cobalt-isel"
#define PASS_NAME "Cobalt DAG->DAG Instruction Selection"

namespace {
class CobaltDAGToDAGISel : public SelectionDAGISel {
public:
  CobaltDAGToDAGISel() = delete;

  CobaltDAGToDAGISel(CobaltTargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

private:
#include "CobaltGenDAGISel.inc"

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
  case ISD::FrameIndex: {
    // Bring-up fallback: Cobalt does not have real stack addressing yet. Use
    // a null address placeholder so non-promoted address temps do not abort
    // instruction selection while scalar lowering is still immature. Do not
    // replace this with a chained CopyFromReg: FrameIndex is a pure value node
    // and mixing chain results here can corrupt SelectionDAG CSE bookkeeping.
    SDValue Zero = CurDAG->getTargetConstant(0, DL, MVT::i32);
    CurDAG->SelectNodeTo(N, Cobalt::VMOVI, MVT::i32, Zero);
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
