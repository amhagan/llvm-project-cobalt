//===-- Cobalt.h - Top-level interface for Cobalt ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_COBALT_COBALT_H
#define LLVM_LIB_TARGET_COBALT_COBALT_H

#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class CobaltTargetMachine;
class FunctionPass;
class PassRegistry;

namespace CobaltAS {
constexpr unsigned Workgroup = 3;
constexpr unsigned DescriptorBase = 8;
constexpr unsigned DescriptorCount = 4;
} // namespace CobaltAS

FunctionPass *createCobaltISelDag(CobaltTargetMachine &TM,
                                  CodeGenOptLevel OptLevel);

void initializeCobaltAsmPrinterPass(PassRegistry &);
void initializeCobaltDAGToDAGISelLegacyPass(PassRegistry &);

} // namespace llvm

#endif // LLVM_LIB_TARGET_COBALT_COBALT_H
