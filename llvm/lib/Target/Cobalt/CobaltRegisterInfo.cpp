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
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/MathExtras.h"

#define GET_REGINFO_TARGET_DESC
#include "CobaltGenRegisterInfo.inc"

using namespace llvm;

namespace {
constexpr uint64_t PrivateScratchWords = 256;

uint64_t getPrivateFrameObjectByteOffset(MachineFunction &MF, int FrameIndex) {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t Offset = 0;
  for (int I = MFI.getObjectIndexBegin(), E = MFI.getObjectIndexEnd(); I != E;
       ++I) {
    // Workgroup globals also become frame objects during SelectionDAG
    // lowering, but they live in CU LDS and have a separate address space.
    // Only register-allocation spill slots consume per-lane private scratch.
    if (I < 0 || MFI.isDeadObjectIndex(I) || !MFI.isSpillSlotObjectIndex(I))
      continue;
    const int64_t Size = MFI.getObjectSize(I);
    if (Size <= 0)
      report_fatal_error("Cobalt private frame objects must have fixed size");
    Offset = alignTo(Offset, MFI.getObjectAlign(I));
    if (I == FrameIndex)
      return Offset;
    Offset += static_cast<uint64_t>(Size);
  }
  report_fatal_error("Cobalt spill frame index was not in MachineFrameInfo");
}
} // namespace

CobaltRegisterInfo::CobaltRegisterInfo()
    : CobaltGenRegisterInfo(/*RA=*/Cobalt::R0, /*DwarfFlavour=*/0,
                            /*EHFlavour=*/0, /*PC=*/0) {}

const MCPhysReg *
CobaltRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const MCPhysReg NoCalleeSavedRegs[] = {0};
  return NoCalleeSavedRegs;
}

BitVector CobaltRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(Cobalt::R14);
  Reserved.set(Cobalt::R15);
  return Reserved;
}

bool CobaltRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                             int SPAdj, unsigned FIOperandNum,
                                             RegScavenger *RS) const {
  (void)SPAdj;
  (void)RS;
  MachineOperand &FrameOperand = MI->getOperand(FIOperandNum);
  const int FrameIndex = FrameOperand.getIndex();
  const uint64_t ByteOffset =
      getPrivateFrameObjectByteOffset(*MI->getMF(), FrameIndex);
  if ((ByteOffset & 3u) != 0)
    report_fatal_error("Cobalt private spill offset is not word aligned");
  const uint64_t WordOffset = ByteOffset / 4u;
  if (WordOffset >= PrivateScratchWords)
    report_fatal_error(
        "Cobalt private spill frame exceeds 256 words per lane");
  FrameOperand.ChangeToImmediate(WordOffset);
  return false;
}

Register CobaltRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return Cobalt::R0;
}
