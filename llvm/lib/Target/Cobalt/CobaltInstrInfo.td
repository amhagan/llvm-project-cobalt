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

def brtarget : Operand<OtherVT>;

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
def VMOVI : CobaltInst<0x04,
                       (outs VGPR32:$rd),
                       (ins i32imm:$imm),
                       "vmovi $rd, $imm",
                       [(set VGPR32:$rd, (i32 imm:$imm))]> {
  bits<4> rd;
  bits<16> imm;

  let Inst{25-22} = rd;
  let Inst{15-0} = imm;
}
def VCMPULT : CobaltRRR<0x05, "vcmpult",
                        [(set VGPR32:$rd,
                              (setult VGPR32:$ra, VGPR32:$rb))]>;
def VCMPULE : CobaltRRR<0x06, "vcmpule",
                        [(set VGPR32:$rd,
                              (setule VGPR32:$ra, VGPR32:$rb))]>;
def VCMPUGE : CobaltRRR<0x07, "vcmpuge",
                        [(set VGPR32:$rd,
                              (setuge VGPR32:$ra, VGPR32:$rb))]>;
def : Pat<(setugt VGPR32:$ra, VGPR32:$rb),
          (VCMPULT VGPR32:$rb, VGPR32:$ra)>;
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
  let Inst{17-14} = 15;
  // LLVM pointer memory is Vulkan descriptor memory during backend bring-up.
  // Scratch memory will get explicit target nodes once stack/private memory is
  // modelled for real.
  let Inst{13} = 1;
  let Inst{12} = 1;
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
  let Inst{17-14} = 15;
  let Inst{13} = 1;
  let Inst{12} = 1;
  // Bring-up convention: generic LLVM stores target SSBO binding 1, while
  // generic loads target SSBO binding 0. A real address-space/binding lowering
  // pass should replace this fixed policy.
  let Inst{0} = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VBR : CobaltInst<0x08,
                     (outs),
                     (ins i32imm:$imm),
                     "vbr $imm"> {
  bits<16> imm;

  let Inst{15-0} = imm;
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
  let hasSideEffects = 1;
}

def VBRNZ : CobaltInst<0x09,
                       (outs),
                       (ins VGPR32:$ra, i32imm:$imm),
                       "vbrnz $ra, $imm"> {
  bits<4> ra;
  bits<16> imm;

  let Inst{21-18} = ra;
  let Inst{15-0} = imm;
  let isBranch = 1;
  let isTerminator = 1;
  let hasSideEffects = 1;
}

// Machine CFG pseudos. The asm printer resolves MBB targets into CobaltISA's
// PC-relative immediate branch encoding because the bring-up object writer does
// not implement relocations yet.
def BR : CobaltInst<0x00,
                    (outs),
                    (ins brtarget:$target),
                    "br $target",
                    [(br bb:$target)]> {
  let isPseudo = 1;
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

def BRCOND : CobaltInst<0x00,
                        (outs),
                        (ins VGPR32:$cond, brtarget:$target),
                        "brnz $cond, $target",
                        [(brcond VGPR32:$cond, bb:$target)]> {
  let isPseudo = 1;
  let isBranch = 1;
  let isTerminator = 1;
}
