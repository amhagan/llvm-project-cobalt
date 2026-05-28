//===-- CobaltAsmPrinter.cpp - Cobalt LLVM assembly writer ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Cobalt.h"
#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "TargetInfo/CobaltTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
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
static int64_t getRawPC(const MachineFunction *MF,
                        const MachineInstr *NeedleMI,
                        const MachineBasicBlock *NeedleMBB) {
  int64_t PC = 0;
  for (const MachineBasicBlock &MBB : *MF) {
    if (&MBB == NeedleMBB)
      return PC;
    for (const MachineInstr &Instr : MBB) {
      if (Instr.isDebugInstr())
        continue;
      if (&Instr == NeedleMI)
        return PC;
      ++PC;
    }
  }
  return -1;
}

static bool isBackwardBRCOND(const MachineInstr &MI) {
  if (MI.getOpcode() != Cobalt::BRCOND)
    return false;

  const MachineFunction *MF = MI.getParent()->getParent();
  const int64_t CurrentPC = getRawPC(MF, &MI, nullptr);
  const int64_t TargetPC = getRawPC(MF, nullptr, MI.getOperand(1).getMBB());
  if (CurrentPC < 0 || TargetPC < 0)
    report_fatal_error("Cobalt branch target was not in the machine function");
  return TargetPC < CurrentPC;
}

static unsigned getExpandedInstrSize(const MachineInstr &MI) {
  if (MI.isDebugInstr())
    return 0;
  return isBackwardBRCOND(MI) ? 3u : 1u;
}

static int64_t getExpandedPC(const MachineFunction *MF,
                             const MachineInstr *NeedleMI,
                             const MachineBasicBlock *NeedleMBB) {
  int64_t PC = 0;
  for (const MachineBasicBlock &MBB : *MF) {
    if (&MBB == NeedleMBB)
      return PC;
    for (const MachineInstr &Instr : MBB) {
      if (Instr.isDebugInstr())
        continue;
      if (&Instr == NeedleMI)
        return PC;
      PC += getExpandedInstrSize(Instr);
    }
  }
  return -1;
}

static int64_t getBranchDelta(const MachineInstr *MI,
                              const MachineBasicBlock *TargetMBB) {
  const MachineFunction *MF = MI->getParent()->getParent();
  const int64_t CurrentPC = getExpandedPC(MF, MI, nullptr);
  const int64_t TargetPC = getExpandedPC(MF, nullptr, TargetMBB);

  if (CurrentPC < 0 || TargetPC < 0)
    report_fatal_error("Cobalt branch target was not in the machine function");

  const int64_t Delta = TargetPC - CurrentPC;
  if (Delta < -32768 || Delta > 32767)
    report_fatal_error("Cobalt branch target is out of imm16 range");
  return Delta;
}

class CobaltAsmPrinter : public AsmPrinter {
public:
  CobaltAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "Cobalt Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override {
    MCInst TmpInst;
    if (MI->getOpcode() == Cobalt::BR) {
      TmpInst.setOpcode(Cobalt::VBR);
      TmpInst.addOperand(
          MCOperand::createImm(getBranchDelta(MI, MI->getOperand(0).getMBB())));
      EmitToStreamer(*OutStreamer, TmpInst);
      return;
    }
    if (MI->getOpcode() == Cobalt::BRCOND) {
      const int64_t Delta = getBranchDelta(MI, MI->getOperand(1).getMBB());
      if (Delta < 0) {
        TmpInst.setOpcode(Cobalt::VBRNZ);
        TmpInst.addOperand(MCOperand::createReg(MI->getOperand(0).getReg()));
        TmpInst.addOperand(MCOperand::createImm(2));
        EmitToStreamer(*OutStreamer, TmpInst);

        TmpInst = MCInst();
        TmpInst.setOpcode(Cobalt::VBR);
        TmpInst.addOperand(MCOperand::createImm(2));
        EmitToStreamer(*OutStreamer, TmpInst);

        TmpInst = MCInst();
        TmpInst.setOpcode(Cobalt::VBR);
        TmpInst.addOperand(MCOperand::createImm(Delta - 2));
        EmitToStreamer(*OutStreamer, TmpInst);
        return;
      }

      TmpInst.setOpcode(Cobalt::VBRNZ);
      TmpInst.addOperand(MCOperand::createReg(MI->getOperand(0).getReg()));
      TmpInst.addOperand(MCOperand::createImm(Delta));
      EmitToStreamer(*OutStreamer, TmpInst);
      return;
    }

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
