//===-- CobaltMCCodeEmitter.cpp - Cobalt machine code emitter ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/EndianStream.h"

using namespace llvm;

#define DEBUG_TYPE "mccodeemitter"

namespace {
class CobaltMCCodeEmitter : public MCCodeEmitter {
  MCContext &Ctx;

public:
  CobaltMCCodeEmitter(MCContext &Ctx, const MCInstrInfo &MCII) : Ctx(Ctx) {}

  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;
};
} // namespace

unsigned
CobaltMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                       SmallVectorImpl<MCFixup> &Fixups,
                                       const MCSubtargetInfo &STI) const {
  if (MO.isReg()) {
    switch (MO.getReg()) {
    case Cobalt::R0:
      return 0;
    case Cobalt::R1:
      return 1;
    case Cobalt::R2:
      return 2;
    case Cobalt::R3:
      return 3;
    case Cobalt::R4:
      return 4;
    case Cobalt::R5:
      return 5;
    case Cobalt::R6:
      return 6;
    case Cobalt::R7:
      return 7;
    case Cobalt::R8:
      return 8;
    case Cobalt::R9:
      return 9;
    case Cobalt::R10:
      return 10;
    case Cobalt::R11:
      return 11;
    case Cobalt::R12:
      return 12;
    case Cobalt::R13:
      return 13;
    case Cobalt::R14:
      return 14;
    case Cobalt::R15:
      return 15;
    default:
      Ctx.reportError(SMLoc(), "unknown Cobalt register operand");
      return 0;
    }
  }
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  Ctx.reportError(SMLoc(), "Cobalt relocatable operands are not implemented");
  return 0;
}

void CobaltMCCodeEmitter::encodeInstruction(const MCInst &MI,
                                            SmallVectorImpl<char> &CB,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const {
  uint32_t Bits = static_cast<uint32_t>(getBinaryCodeForInstr(MI, Fixups, STI));
  support::endian::write<uint32_t>(CB, Bits, llvm::endianness::little);
}

MCCodeEmitter *llvm::createCobaltMCCodeEmitter(const MCInstrInfo &MCII,
                                               MCContext &Ctx) {
  return new CobaltMCCodeEmitter(Ctx, MCII);
}

#include "CobaltGenMCCodeEmitter.inc"
