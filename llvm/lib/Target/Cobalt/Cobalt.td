//===-- Cobalt.td - Describe the Cobalt target machine -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

include "CobaltRegisterInfo.td"
include "CobaltInstrInfo.td"

defm : RemapAllTargetPseudoPointerOperands<VGPR32>;

def CobaltInstrInfo : InstrInfo;

def : Processor<"generic", NoItineraries, []>;

def Cobalt : Target {
  let InstructionSet = CobaltInstrInfo;
}
