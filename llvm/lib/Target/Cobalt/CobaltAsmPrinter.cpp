//===-- CobaltAsmPrinter.cpp - Cobalt LLVM assembly writer ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Cobalt.h"
#include "TargetInfo/CobaltTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {
class CobaltAsmPrinter : public AsmPrinter {
public:
  CobaltAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "Cobalt Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override {
    MCInst TmpInst;
    TmpInst.setOpcode(MI->getOpcode());
    for (const MachineOperand &MO : MI->operands()) {
      if (MO.isReg()) {
        if (MO.getReg())
          TmpInst.addOperand(MCOperand::createReg(MO.getReg()));
      } else if (MO.isImm()) {
        TmpInst.addOperand(MCOperand::createImm(MO.getImm()));
      } else if (MO.isMBB()) {
        TmpInst.addOperand(MCOperand::createExpr(
            MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), OutContext)));
      }
    }
    EmitToStreamer(*OutStreamer, TmpInst);
  }

  static char ID;
};
} // namespace

char CobaltAsmPrinter::ID = 0;

INITIALIZE_PASS(CobaltAsmPrinter, "cobalt-asm-printer",
                "Cobalt Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeCobaltAsmPrinter() {
  RegisterAsmPrinter<CobaltAsmPrinter> X(getTheCobaltTarget());
}
