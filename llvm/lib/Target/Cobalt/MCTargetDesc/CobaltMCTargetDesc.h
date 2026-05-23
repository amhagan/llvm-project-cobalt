//===-- CobaltMCTargetDesc.h - Cobalt target descriptions ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_COBALT_MCTARGETDESC_COBALTMCTARGETDESC_H
#define LLVM_LIB_TARGET_COBALT_MCTARGETDESC_COBALTMCTARGETDESC_H

namespace llvm {
class MCInstrInfo;

MCInstrInfo *createCobaltMCInstrInfo();
} // namespace llvm

#define GET_REGINFO_ENUM
#include "CobaltGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "CobaltGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "CobaltGenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_COBALT_MCTARGETDESC_COBALTMCTARGETDESC_H
