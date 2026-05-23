//===-- CobaltSubtarget.h - Cobalt subtarget info -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_COBALT_COBALTSUBTARGET_H
#define LLVM_LIB_TARGET_COBALT_COBALTSUBTARGET_H

#include "CobaltFrameLowering.h"
#include "CobaltInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include <string>

#define GET_SUBTARGETINFO_HEADER
#include "CobaltGenSubtargetInfo.inc"

namespace llvm {

class CobaltTargetMachine;

class CobaltSubtarget : public CobaltGenSubtargetInfo {
  CobaltInstrInfo InstrInfo;
  CobaltFrameLowering FrameLowering;

public:
  CobaltSubtarget(const Triple &TT, const std::string &CPU,
                  const std::string &FS, const CobaltTargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const CobaltInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const CobaltFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const CobaltRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_COBALT_COBALTSUBTARGET_H
