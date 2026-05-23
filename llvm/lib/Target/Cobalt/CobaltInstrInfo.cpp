//===-- CobaltInstrInfo.cpp - Cobalt instruction info ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltInstrInfo.h"
#include "CobaltSubtarget.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "CobaltGenInstrInfo.inc"

using namespace llvm;

CobaltInstrInfo::CobaltInstrInfo(const CobaltSubtarget &STI)
    : CobaltGenInstrInfo(STI, RI, /*CFSetupOpcode=*/0,
                         /*CFDestroyOpcode=*/0, /*CatchRetOpcode=*/0,
                         /*ReturnOpcode=*/0) {}
