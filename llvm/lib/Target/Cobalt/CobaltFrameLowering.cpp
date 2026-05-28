//===-- CobaltFrameLowering.cpp - Cobalt frame lowering --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltFrameLowering.h"
#include "CobaltInstrInfo.h"
#include "CobaltSubtarget.h"
#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

void CobaltFrameLowering::emitPrologue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  const auto &STI = MF.getSubtarget<CobaltSubtarget>();
  const TargetInstrInfo &TII = *STI.getInstrInfo();
  MachineBasicBlock::iterator Insert = MBB.begin();
  BuildMI(MBB, Insert, DebugLoc(), TII.get(Cobalt::VMOVI), Cobalt::R15)
      .addImm(0);
}

void CobaltFrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {}

bool CobaltFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return false;
}
