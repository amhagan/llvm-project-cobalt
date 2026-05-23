//===-- CobaltELFObjectWriter.cpp - Cobalt ELF writer ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class CobaltELFObjectWriter : public MCELFObjectTargetWriter {
public:
  CobaltELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_NONE,
                                /*HasRelocationAddend=*/true) {}

protected:
  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    llvm_unreachable("Cobalt relocations are not implemented");
  }
};
} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createCobaltELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<CobaltELFObjectWriter>(OSABI);
}
