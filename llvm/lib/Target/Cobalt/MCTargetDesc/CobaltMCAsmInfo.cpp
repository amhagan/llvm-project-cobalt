//===-- CobaltMCAsmInfo.cpp - Cobalt asm properties -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltMCAsmInfo.h"

using namespace llvm;

void CobaltMCAsmInfo::anchor() {}

CobaltMCAsmInfo::CobaltMCAsmInfo(const Triple &TT,
                                 const MCTargetOptions &Options)
    : MCAsmInfoELF(Options) {
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;
  CommentString = ";";
  AlignmentIsInBytes = true;
  SupportsDebugInformation = false;
  ExceptionsType = ExceptionHandling::DwarfCFI;
}
