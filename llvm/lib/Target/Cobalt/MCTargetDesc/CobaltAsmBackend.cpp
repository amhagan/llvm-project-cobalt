//===-- CobaltAsmBackend.cpp - Cobalt assembler backend --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/EndianStream.h"

using namespace llvm;

namespace {
class CobaltAsmBackend : public MCAsmBackend {
public:
  CobaltAsmBackend() : MCAsmBackend(llvm::endianness::little) {}

  void applyFixup(const MCFragment &, const MCFixup &, const MCValue &Target,
                  uint8_t *Data, uint64_t Value, bool IsResolved) override {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createCobaltELFObjectWriter(/*OSABI=*/0);
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    return MCAsmBackend::getFixupKindInfo(Kind);
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    if ((Count % 4) != 0)
      return false;

    for (uint64_t I = 0; I < Count; I += 4)
      support::endian::write<uint32_t>(OS, 0, llvm::endianness::little);
    return true;
  }
};
} // namespace

MCAsmBackend *llvm::createCobaltAsmBackend(const Target &T,
                                           const MCSubtargetInfo &STI,
                                           const MCRegisterInfo &MRI,
                                           const MCTargetOptions &Options) {
  return new CobaltAsmBackend();
}
