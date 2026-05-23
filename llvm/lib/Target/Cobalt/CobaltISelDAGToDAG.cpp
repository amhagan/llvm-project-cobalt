//===-- CobaltISelDAGToDAG.cpp - Cobalt DAG selector -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Cobalt.h"
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

  SelectCode(N);
}

FunctionPass *llvm::createCobaltISelDag(CobaltTargetMachine &TM,
                                        CodeGenOptLevel OptLevel) {
  return new CobaltDAGToDAGISelLegacy(TM, OptLevel);
}
