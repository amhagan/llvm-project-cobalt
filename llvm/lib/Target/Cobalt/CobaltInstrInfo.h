//===-- CobaltInstrInfo.h - Cobalt instruction info ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_COBALT_COBALTINSTRINFO_H
#define LLVM_LIB_TARGET_COBALT_COBALTINSTRINFO_H

#include "CobaltRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "CobaltGenInstrInfo.inc"

namespace llvm {

class CobaltSubtarget;

class CobaltInstrInfo : public CobaltGenInstrInfo {
  CobaltRegisterInfo RI;

public:
  explicit CobaltInstrInfo(const CobaltSubtarget &STI);

  const CobaltRegisterInfo &getRegisterInfo() const { return RI; }

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, Register SrcReg,
                           bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC, Register VReg,
                           MachineInstr::MIFlag Flags) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI, Register DestReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            Register VReg, unsigned SubReg,
                            MachineInstr::MIFlag Flags) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_COBALT_COBALTINSTRINFO_H
