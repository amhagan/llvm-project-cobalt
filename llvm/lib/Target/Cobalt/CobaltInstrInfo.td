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
def VMOVIF : CobaltInst<0x04,
                        (outs FGPR32:$rd),
                        (ins i32imm:$imm),
                        "vmovi $rd, $imm"> {
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
def : Pat<(seteq VGPR32:$ra, VGPR32:$rb),
          (VAND (VCMPULE VGPR32:$ra, VGPR32:$rb),
                (VCMPUGE VGPR32:$ra, VGPR32:$rb))>;
def : Pat<(setne VGPR32:$ra, VGPR32:$rb),
          (VOR (VCMPULT VGPR32:$ra, VGPR32:$rb),
               (VCMPULT VGPR32:$rb, VGPR32:$ra))>;
class CobaltFRR<bits<6> Opcode, string Mnemonic, list<dag> Pat = []>
    : CobaltInst<Opcode,
                 (outs FGPR32:$rd),
                 (ins FGPR32:$ra, FGPR32:$rb),
                 !strconcat(Mnemonic, " $rd, $ra, $rb"),
                 Pat> {
  bits<4> rd;
  bits<4> ra;
  bits<4> rb;

  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{17-14} = rb;
}

def VFADD : CobaltFRR<0x0d, "vfadd",
                      [(set FGPR32:$rd, (fadd FGPR32:$ra, FGPR32:$rb))]>;
def VFMUL : CobaltFRR<0x0e, "vfmul",
                      [(set FGPR32:$rd, (fmul FGPR32:$ra, FGPR32:$rb))]>;

class CobaltFCmp<bits<6> Opcode, string Mnemonic, list<dag> Pat = []>
    : CobaltInst<Opcode,
                 (outs VGPR32:$rd),
                 (ins FGPR32:$ra, FGPR32:$rb),
                 !strconcat(Mnemonic, " $rd, $ra, $rb"),
                 Pat> {
  bits<4> rd;
  bits<4> ra;
  bits<4> rb;

  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{17-14} = rb;
}

def VFCMPEQ : CobaltFCmp<0x1b, "vfcmpeq",
                         [(set VGPR32:$rd,
                               (setoeq FGPR32:$ra, FGPR32:$rb))]>;
def VFCMPNE : CobaltFCmp<0x1c, "vfcmpne",
                         [(set VGPR32:$rd,
                               (setune FGPR32:$ra, FGPR32:$rb))]>;
def VFCMPLT : CobaltFCmp<0x1d, "vfcmplt",
                         [(set VGPR32:$rd,
                               (setolt FGPR32:$ra, FGPR32:$rb))]>;
def VFCMPLE : CobaltFCmp<0x1e, "vfcmple",
                         [(set VGPR32:$rd,
                               (setole FGPR32:$ra, FGPR32:$rb))]>;
def : Pat<(setogt FGPR32:$ra, FGPR32:$rb),
          (VFCMPLT FGPR32:$rb, FGPR32:$ra)>;
def : Pat<(setoge FGPR32:$ra, FGPR32:$rb),
          (VFCMPLE FGPR32:$rb, FGPR32:$ra)>;

def VSELECT : CobaltInst<0x1f,
                         (outs FGPR32:$rd),
                         (ins VGPR32:$condition, FGPR32:$true_value,
                              FGPR32:$false_value),
                         "vselect $rd, $condition, $true_value",
                         [(set FGPR32:$rd,
                               (select VGPR32:$condition,
                                       FGPR32:$true_value,
                                       FGPR32:$false_value))]> {
  bits<4> rd;
  bits<4> condition;
  bits<4> true_value;

  // Two-address form: rd supplies false_value before writeback.
  let Constraints = "$rd = $false_value";
  let Inst{25-22} = rd;
  let Inst{21-18} = condition;
  let Inst{17-14} = true_value;
}

def VSPLATSGPR : CobaltInst<0x16,
                            (outs VGPR32:$rd),
                            (ins i32imm:$sgpr),
                            "vsplatsgpr $rd, $sgpr"> {
  bits<4> rd;
  bits<16> sgpr;

  // Broadcast the current waveslot's scalar launch/user-data register into a
  // VGPR for existing vector-only datapaths.
  let Inst{25-22} = rd;
  let Inst{15-0} = sgpr;
  let hasSideEffects = 1;
}
class CobaltRRSgpr<bits<6> Opcode, string Mnemonic>
    : CobaltInst<Opcode,
                 (outs VGPR32:$rd),
                 (ins VGPR32:$ra, i32imm:$sgpr),
                 !strconcat(Mnemonic, " $rd, $ra, $sgpr")> {
  bits<4> rd;
  bits<4> ra;
  bits<16> sgpr;

  // rd[lane] = op(ra[lane], s[sgpr]) for the current resident waveslot.
  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{15-0} = sgpr;
}
def VADDSGPR : CobaltRRSgpr<0x17, "vaddsgpr">;
def VSUBSGPR : CobaltRRSgpr<0x18, "vsubsgpr">;
def VANDSGPR : CobaltRRSgpr<0x19, "vandsgpr">;
def VORSGPR  : CobaltRRSgpr<0x1a, "vorsgpr">;
def VCMPULTSGPR : CobaltRRSgpr<0x20, "vcmpultsgpr">;
def VCMPULESGPR : CobaltRRSgpr<0x21, "vcmpulesgpr">;
def VCMPUGESGPR : CobaltRRSgpr<0x22, "vcmpugesgpr">;
def VXORSGPR : CobaltRRSgpr<0x24, "vxorsgpr">;
def VMULSGPR : CobaltRRSgpr<0x25, "vmulsgpr">;

class CobaltFRSgpr<bits<6> Opcode, string Mnemonic>
    : CobaltInst<Opcode,
                 (outs FGPR32:$rd),
                 (ins FGPR32:$ra, i32imm:$sgpr),
                 !strconcat(Mnemonic, " $rd, $ra, $sgpr")> {
  bits<4> rd;
  bits<4> ra;
  bits<16> sgpr;

  // The SGPR contains raw IEEE-754 binary32 bits; no scalar conversion occurs.
  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{15-0} = sgpr;
}

class CobaltFCmpSgpr<bits<6> Opcode, string Mnemonic>
    : CobaltInst<Opcode,
                 (outs VGPR32:$rd),
                 (ins FGPR32:$ra, i32imm:$sgpr),
                 !strconcat(Mnemonic, " $rd, $ra, $sgpr")> {
  bits<4> rd;
  bits<4> ra;
  bits<16> sgpr;

  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{15-0} = sgpr;
}

def VFADDSGPR : CobaltFRSgpr<0x26, "vfaddsgpr">;
def VFMULSGPR : CobaltFRSgpr<0x27, "vfmulsgpr">;
def VFCMPEQSGPR : CobaltFCmpSgpr<0x28, "vfcmpeqsgpr">;
def VFCMPNESGPR : CobaltFCmpSgpr<0x29, "vfcmpnesgpr">;
def VFCMPLTSGPR : CobaltFCmpSgpr<0x2a, "vfcmpltsgpr">;
def VFCMPLESGPR : CobaltFCmpSgpr<0x2b, "vfcmplesgpr">;
def VLSHRI : CobaltInst<0x23,
                        (outs VGPR32:$rd),
                        (ins VGPR32:$ra, i32imm:$imm),
                        "vlshri $rd, $ra, $imm"> {
  bits<4> rd;
  bits<4> ra;
  bits<5> imm;

  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{4-0} = imm;
}
def VMOVHI : CobaltInst<0x0f,
                        (outs VGPR32:$rd),
                        (ins VGPR32:$ra, i32imm:$imm),
                        "vmovhi $rd, $ra, $imm"> {
  bits<4> rd;
  bits<4> ra;
  bits<16> imm;

  // rd = (ra & 0xffff) | (imm << 16). This gives the backend a compact
  // two-instruction materialization path for arbitrary 32-bit lane constants.
  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{15-0} = imm;
}
def VMOVHIF : CobaltInst<0x0f,
                         (outs FGPR32:$rd),
                         (ins FGPR32:$ra, i32imm:$imm),
                         "vmovhi $rd, $ra, $imm"> {
  bits<4> rd;
  bits<4> ra;
  bits<16> imm;

  let Inst{25-22} = rd;
  let Inst{21-18} = ra;
  let Inst{15-0} = imm;
}

def VLD : CobaltInst<0x10,
                     (outs VGPR32:$rd),
                     (ins VGPR32:$addr, i32imm:$binding),
                     "vld $rd, [$addr], binding $binding"> {
  let Constraints = "@earlyclobber $rd";

  bits<4> rd;
  bits<4> addr;
  bits<2> binding;

  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = 15;
  // LLVM pointer memory is Vulkan descriptor memory during backend bring-up.
  // Scratch memory will get explicit target nodes once stack/private memory is
  // modelled for real.
  let Inst{13} = 1;
  let Inst{12} = 1;
  let Inst{1-0} = binding;
  let mayLoad = 1;
}

def VLDF : CobaltInst<0x10,
                      (outs FGPR32:$rd),
                      (ins VGPR32:$addr, i32imm:$binding),
                      "vld $rd, [$addr], binding $binding"> {
  let Constraints = "@earlyclobber $rd";

  bits<4> rd;
  bits<4> addr;
  bits<2> binding;

  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = 15;
  let Inst{13} = 1;
  let Inst{12} = 1;
  let Inst{1-0} = binding;
  let mayLoad = 1;
}

def VST : CobaltInst<0x11,
                     (outs),
                     (ins VGPR32:$src, VGPR32:$addr, i32imm:$binding),
                     "vst $src, [$addr], binding $binding"> {
  bits<4> src;
  bits<4> addr;
  bits<2> binding;

  let Inst{25-22} = src;
  let Inst{21-18} = addr;
  let Inst{17-14} = 15;
  let Inst{13} = 1;
  let Inst{12} = 1;
  let Inst{1-0} = binding;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VSTF : CobaltInst<0x11,
                      (outs),
                      (ins FGPR32:$src, VGPR32:$addr, i32imm:$binding),
                      "vst $src, [$addr], binding $binding"> {
  bits<4> src;
  bits<4> addr;
  bits<2> binding;

  let Inst{25-22} = src;
  let Inst{21-18} = addr;
  let Inst{17-14} = 15;
  let Inst{13} = 1;
  let Inst{12} = 1;
  let Inst{1-0} = binding;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VLDS : CobaltInst<0x12,
                      (outs VGPR32:$rd),
                      (ins VGPR32:$addr),
                      "vlds $rd, [$addr]"> {
  bits<4> rd;
  bits<4> addr;

  // Workgroup/shared memory is byte-addressed. The current b32 form requires
  // bits [1:0] of $addr to be zero; unaligned behavior is undefined during
  // bring-up. Future b8/b16/b64 forms should keep the same byte-offset model.
  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = 15;
  let mayLoad = 1;
}

def VSTS : CobaltInst<0x13,
                      (outs),
                      (ins VGPR32:$src, VGPR32:$addr),
                      "vsts $src, [$addr]"> {
  bits<4> src;
  bits<4> addr;

  // Workgroup/shared memory is byte-addressed. The current b32 form requires
  // bits [1:0] of $addr to be zero; unaligned behavior is undefined during
  // bring-up. Future b8/b16/b64 forms should keep the same byte-offset model.
  let Inst{25-22} = src;
  let Inst{21-18} = addr;
  let Inst{17-14} = 15;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VBARRIER : CobaltInst<0x14,
                          (outs),
                          (ins),
                          "vbarrier",
                          [(atomic_fence timm, timm)]> {
  let isBarrier = 1;
  let hasSideEffects = 1;
}

def VATOMIADD : CobaltInst<0x15,
                           (outs VGPR32:$rd),
                           (ins VGPR32:$addr, VGPR32:$val, i32imm:$binding),
                           "vatomiadd $rd, [$addr], $val, binding $binding"> {
  bits<4> rd;
  bits<4> addr;
  bits<4> val;
  bits<2> binding;

  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = val;
  let Inst{13} = 1;
  let Inst{12} = 1;
  let Inst{1-0} = binding;
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VATOMIADDS : CobaltInst<0x15,
                            (outs VGPR32:$rd),
                            (ins VGPR32:$addr, VGPR32:$val),
                            "vatomiadds $rd, [$addr], $val"> {
  bits<4> rd;
  bits<4> addr;
  bits<4> val;

  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = val;
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class CobaltAtomicExternal<bits<4> AtomicOp, string Mnemonic>
    : CobaltInst<0x2c,
                 (outs VGPR32:$rd),
                 (ins VGPR32:$addr, VGPR32:$val, i32imm:$binding),
                 !strconcat(Mnemonic,
                            " $rd, [$addr], $val, binding $binding")> {
  bits<4> rd;
  bits<4> addr;
  bits<4> val;
  bits<2> binding;

  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = val;
  let Inst{13} = 1;
  let Inst{12} = 1;
  let Inst{11-8} = AtomicOp;
  let Inst{1-0} = binding;
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class CobaltAtomicShared<bits<4> AtomicOp, string Mnemonic>
    : CobaltInst<0x2c,
                 (outs VGPR32:$rd),
                 (ins VGPR32:$addr, VGPR32:$val),
                 !strconcat(Mnemonic, " $rd, [$addr], $val")> {
  bits<4> rd;
  bits<4> addr;
  bits<4> val;

  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = val;
  let Inst{11-8} = AtomicOp;
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VATOMXCHG  : CobaltAtomicExternal<1, "vatomxchg">;
def VATOMXCHGS : CobaltAtomicShared<1, "vatomxchgs">;
def VATOMSMIN  : CobaltAtomicExternal<3, "vatomsmin">;
def VATOMSMINS : CobaltAtomicShared<3, "vatomsmins">;
def VATOMUMIN  : CobaltAtomicExternal<4, "vatomumin">;
def VATOMUMINS : CobaltAtomicShared<4, "vatomumins">;
def VATOMSMAX  : CobaltAtomicExternal<5, "vatomsmax">;
def VATOMSMAXS : CobaltAtomicShared<5, "vatomsmaxs">;
def VATOMUMAX  : CobaltAtomicExternal<6, "vatomumax">;
def VATOMUMAXS : CobaltAtomicShared<6, "vatomumaxs">;
def VATOMAND   : CobaltAtomicExternal<7, "vatomand">;
def VATOMANDS  : CobaltAtomicShared<7, "vatomands">;
def VATOMOR    : CobaltAtomicExternal<8, "vatomor">;
def VATOMORS   : CobaltAtomicShared<8, "vatomors">;
def VATOMXOR   : CobaltAtomicExternal<9, "vatomxor">;
def VATOMXORS  : CobaltAtomicShared<9, "vatomxors">;

class CobaltAtomicCmpXchgExternal<string Mnemonic>
    : CobaltInst<0x2c,
                 (outs VGPR32:$rd),
                 (ins VGPR32:$addr, VGPR32:$val, VGPR32:$compare,
                      i32imm:$binding),
                 !strconcat(Mnemonic,
                            " $rd, [$addr], $val, $compare, binding $binding")> {
  bits<4> rd;
  bits<4> addr;
  bits<4> val;
  bits<2> binding;

  let Constraints = "$rd = $compare";
  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = val;
  let Inst{13} = 1;
  let Inst{12} = 1;
  let Inst{11-8} = 2;
  let Inst{1-0} = binding;
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class CobaltAtomicCmpXchgShared<string Mnemonic>
    : CobaltInst<0x2c,
                 (outs VGPR32:$rd),
                 (ins VGPR32:$addr, VGPR32:$val, VGPR32:$compare),
                 !strconcat(Mnemonic,
                            " $rd, [$addr], $val, $compare")> {
  bits<4> rd;
  bits<4> addr;
  bits<4> val;

  let Constraints = "$rd = $compare";
  let Inst{25-22} = rd;
  let Inst{21-18} = addr;
  let Inst{17-14} = val;
  let Inst{11-8} = 2;
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def VATOMCMPXCHG  : CobaltAtomicCmpXchgExternal<"vatomcmpxchg">;
def VATOMCMPXCHGS : CobaltAtomicCmpXchgShared<"vatomcmpxchgs">;

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
