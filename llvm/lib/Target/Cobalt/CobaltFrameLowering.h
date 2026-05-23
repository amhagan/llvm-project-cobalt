//===-- CobaltFrameLowering.h - Cobalt frame lowering ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_COBALT_COBALTFRAMELOWERING_H
#define LLVM_LIB_TARGET_COBALT_COBALTFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/Support/Alignment.h"

namespace llvm {

class CobaltSubtarget;

class CobaltFrameLowering : public TargetFrameLowering {
public:
  explicit CobaltFrameLowering(const CobaltSubtarget &STI)
      : TargetFrameLowering(StackGrowsDown, Align(4), 0) {}

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

protected:
  bool hasFPImpl(const MachineFunction &MF) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_COBALT_COBALTFRAMELOWERING_H
