//===-- CobaltMCTargetDesc.cpp - Cobalt target descriptions ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltMCTargetDesc.h"
#include "TargetInfo/CobaltTargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "CobaltGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "CobaltGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "CobaltGenSubtargetInfo.inc"

MCInstrInfo *llvm::createCobaltMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitCobaltMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createCobaltMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitCobaltMCRegisterInfo(X, /*RA=*/0);
  return X;
}

static MCSubtargetInfo *
createCobaltMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  if (CPU.empty())
    CPU = "generic";
  return createCobaltMCSubtargetInfoImpl(TT, CPU, /*TuneCPU=*/CPU, FS);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeCobaltTargetMC() {
  Target &T = getTheCobaltTarget();

  TargetRegistry::RegisterMCInstrInfo(T, createCobaltMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createCobaltMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createCobaltMCSubtargetInfo);
}
