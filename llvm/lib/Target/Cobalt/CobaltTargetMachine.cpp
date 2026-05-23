//===-- CobaltTargetMachine.cpp - Cobalt target machine --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltTargetMachine.h"
#include "TargetInfo/CobaltTargetInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeCobaltTarget() {
  RegisterTargetMachine<CobaltTargetMachine> X(getTheCobaltTarget());
}

static std::string computeDataLayout(const Triple &TT) {
  // Current hardware-visible Cobalt pointers are 32-bit SDRAM/device offsets.
  // Keep the first backend milestone intentionally conservative.
  return TT.computeDataLayout();
}

CobaltTargetMachine::CobaltTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, computeDataLayout(TT), TT, CPU, FS, Options,
                               RM.value_or(Reloc::Static),
                               getEffectiveCodeModel(CM, CodeModel::Small), OL),
      Subtarget(TT, std::string(CPU), std::string(FS), *this) {}

namespace {
class CobaltPassConfig : public TargetPassConfig {
public:
  CobaltPassConfig(CobaltTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addInstSelector() override;
};
} // namespace

TargetPassConfig *CobaltTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new CobaltPassConfig(*this, PM);
}

bool CobaltPassConfig::addInstSelector() {
  report_fatal_error("Cobalt instruction selection is not implemented yet");
}
