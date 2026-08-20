//===-- CobaltInstrInfo.cpp - Cobalt instruction info ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltInstrInfo.h"
#include "CobaltSubtarget.h"
#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "CobaltGenInstrInfo.inc"

using namespace llvm;

CobaltInstrInfo::CobaltInstrInfo(const CobaltSubtarget &STI)
    : CobaltGenInstrInfo(STI, RI, /*CFSetupOpcode=*/0,
                         /*CFDestroyOpcode=*/0, /*CatchRetOpcode=*/0,
                         /*ReturnOpcode=*/0) {}

void CobaltInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator MI,
                                  const DebugLoc &DL, Register DestReg,
                                  Register SrcReg, bool KillSrc,
                                  bool RenamableDest, bool RenamableSrc) const {
  if (DestReg == SrcReg)
    return;

  if (!Cobalt::VGPR32RegClass.contains(DestReg, SrcReg))
    llvm_unreachable("Cobalt can only copy VGPR32 registers");

  // CobaltISA 1.0 has no register move opcode. OR-ing a source with itself is
  // an exact per-lane copy and uses existing hardware.
  BuildMI(MBB, MI, DL, get(Cobalt::VOR), DestReg)
      .addReg(SrcReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}

void CobaltInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool isKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  (void)VReg;
  const unsigned Opcode = RC == &Cobalt::FGPR32RegClass
                              ? Cobalt::VSTPRIVF
                              : Cobalt::VSTPRIV;
  BuildMI(MBB, MI, DebugLoc(), get(Opcode))
      .addReg(SrcReg, getKillRegState(isKill))
      .addFrameIndex(FrameIndex)
      .setMIFlag(Flags);
}

void CobaltInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                           MachineBasicBlock::iterator MI,
                                           Register DestReg, int FrameIndex,
                                           const TargetRegisterClass *RC,
                                           Register VReg, unsigned SubReg,
                                           MachineInstr::MIFlag Flags) const {
  (void)VReg;
  (void)SubReg;
  const unsigned Opcode = RC == &Cobalt::FGPR32RegClass
                              ? Cobalt::VLDPRIVF
                              : Cobalt::VLDPRIV;
  BuildMI(MBB, MI, DebugLoc(), get(Opcode), DestReg)
      .addFrameIndex(FrameIndex)
      .setMIFlag(Flags);
}
