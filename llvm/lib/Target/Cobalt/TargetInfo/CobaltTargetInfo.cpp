//===-- CobaltTargetInfo.cpp - Cobalt target implementation -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/CobaltTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

Target &llvm::getTheCobaltTarget() {
  static Target TheCobaltTarget;
  return TheCobaltTarget;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeCobaltTargetInfo() {
  RegisterTarget<Triple::cobalt, /*HasJIT=*/false> X(
      getTheCobaltTarget(), "cobalt", "Cobalt Compute ISA", "Cobalt");
}
