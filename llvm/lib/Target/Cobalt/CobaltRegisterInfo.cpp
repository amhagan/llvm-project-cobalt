//===-- CobaltRegisterInfo.cpp - Cobalt register info ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CobaltRegisterInfo.h"
#include "CobaltFrameLowering.h"
#include "MCTargetDesc/CobaltMCTargetDesc.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_REGINFO_TARGET_DESC
#include "CobaltGenRegisterInfo.inc"

using namespace llvm;

CobaltRegisterInfo::CobaltRegisterInfo()
    : CobaltGenRegisterInfo(/*RA=*/Cobalt::R0, /*DwarfFlavour=*/0,
                            /*EHFlavour=*/0, /*PC=*/0) {}

const MCPhysReg *
CobaltRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const MCPhysReg NoCalleeSavedRegs[] = {0};
  return NoCalleeSavedRegs;
}

BitVector CobaltRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  return BitVector(getNumRegs());
}

bool CobaltRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                             int SPAdj, unsigned FIOperandNum,
                                             RegScavenger *RS) const {
  return false;
}

Register CobaltRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return Cobalt::R0;
}
