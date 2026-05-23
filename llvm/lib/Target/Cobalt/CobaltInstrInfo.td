//===-- CobaltInstrInfo.td - Cobalt instruction defs -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "CobaltInstrFormats.td"

//===----------------------------------------------------------------------===//
// Cobalt-specific SelectionDAG nodes.
//===----------------------------------------------------------------------===//

def Cobaltret : SDNode<"CobaltISD::RET", SDTNone,
                       [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def NOP  : CobaltInst<0x00, (outs), (ins), "nop">;
def HALT : CobaltInst<0x3f, (outs), (ins), "halt"> {
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
  let hasSideEffects = 1;
  let Pattern = [(Cobaltret)];
}

class CobaltRRR<bits<6> Opcode, string Mnemonic, list<dag> Pat = []>
    : CobaltInst<Opcode,
                 (outs VGPR32:$rd),
                 (ins VGPR32:$ra, VGPR32:$rb),
                 !strconcat(Mnemonic, " $rd, $ra, $rb"),
                 Pat> {
  bits<4> rd;
  bits<4> ra;
  bits<4> rb;

  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{17-14} = rb;
}

def VADD : CobaltRRR<0x01, "vadd",
                     [(set VGPR32:$rd, (add VGPR32:$ra, VGPR32:$rb))]>;
def VXOR : CobaltRRR<0x02, "vxor",
                     [(set VGPR32:$rd, (xor VGPR32:$ra, VGPR32:$rb))]>;
def VMUL : CobaltRRR<0x03, "vmul",
                     [(set VGPR32:$rd, (mul VGPR32:$ra, VGPR32:$rb))]>;
def VSUB : CobaltRRR<0x0a, "vsub",
                     [(set VGPR32:$rd, (sub VGPR32:$ra, VGPR32:$rb))]>;
def VAND : CobaltRRR<0x0b, "vand",
                     [(set VGPR32:$rd, (and VGPR32:$ra, VGPR32:$rb))]>;
def VOR  : CobaltRRR<0x0c, "vor",
                     [(set VGPR32:$rd, (or VGPR32:$ra, VGPR32:$rb))]>;

def VLD : CobaltInst<0x10,
                     (outs VGPR32:$rd),
                     (ins VGPR32:$addr),
                     "vld $rd, [$addr]",
                     [(set VGPR32:$rd, (load VGPR32:$addr))]> {
  bits<4> rd;
  bits<4> addr;

  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let mayLoad = 1;
}

def VST : CobaltInst<0x11,
                     (outs),
                     (ins VGPR32:$src, VGPR32:$addr),
                     "vst $src, [$addr]",
                     [(store VGPR32:$src, VGPR32:$addr)]> {
  bits<4> src;
  bits<4> addr;

  let Inst{25-22} = src;
  let Inst{21-18} = addr;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VBR : CobaltInst<0x09,
                     (outs),
                     (ins VGPR32:$cond),
                     "vbr $cond"> {
  let isBranch = 1;
  let isTerminator = 1;
  let hasSideEffects = 1;
}
