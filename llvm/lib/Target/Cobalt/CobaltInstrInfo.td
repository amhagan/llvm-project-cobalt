//===-- CobaltInstrInfo.td - Cobalt instruction defs -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "CobaltInstrFormats.td"

def NOP  : CobaltInst<0x00, (outs), (ins), "nop">;
def HALT : CobaltInst<0x01, (outs), (ins), "halt"> {
  let isTerminator = 1;
  let hasSideEffects = 1;
}

class CobaltRRR<bits<8> Opcode, string Mnemonic>
    : CobaltInst<Opcode,
                 (outs VGPR32:$rd),
                 (ins VGPR32:$ra, VGPR32:$rb),
                 !strconcat(Mnemonic, " $rd, $ra, $rb")>;

def VADD : CobaltRRR<0x10, "vadd">;
def VSUB : CobaltRRR<0x11, "vsub">;
def VMUL : CobaltRRR<0x12, "vmul">;
def VAND : CobaltRRR<0x13, "vand">;
def VOR  : CobaltRRR<0x14, "vor">;
def VXOR : CobaltRRR<0x15, "vxor">;

def VLD : CobaltInst<0x20,
                     (outs VGPR32:$rd),
                     (ins VGPR32:$addr),
                     "vld $rd, [$addr]"> {
  let mayLoad = 1;
}

def VST : CobaltInst<0x21,
                     (outs),
                     (ins VGPR32:$src, VGPR32:$addr),
                     "vst $src, [$addr]"> {
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VBR : CobaltInst<0x30,
                     (outs),
                     (ins VGPR32:$cond),
                     "vbr $cond"> {
  let isBranch = 1;
  let isTerminator = 1;
  let hasSideEffects = 1;
}
