//===-- CobaltSubtarget.cpp - Cobalt subtarget info -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltSubtarget.h"
#include "CobaltTargetMachine.h"

#define DEBUG_TYPE "cobalt-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "CobaltGenSubtargetInfo.inc"

using namespace llvm;

CobaltSubtarget::CobaltSubtarget(const Triple &TT, const std::string &CPU,
                                 const std::string &FS,
                                 const CobaltTargetMachine &TM)
    : CobaltGenSubtargetInfo(TT, CPU, CPU, FS), InstrInfo(*this),
      TLInfo(TM, *this), FrameLowering(*this) {
  ParseSubtargetFeatures(CPU, CPU, FS);
}
