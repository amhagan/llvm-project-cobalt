//===-- CobaltRegisterInfo.td - Cobalt register defs -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class CobaltReg<bits<16> Num, string Name> : Register<Name> {
  let Namespace = "Cobalt";
  let HWEncoding = Num;
  let DwarfNumbers = [Num];
}

def R0  : CobaltReg<0,  "r0">;
def R1  : CobaltReg<1,  "r1">;
def R2  : CobaltReg<2,  "r2">;
def R3  : CobaltReg<3,  "r3">;
def R4  : CobaltReg<4,  "r4">;
def R5  : CobaltReg<5,  "r5">;
def R6  : CobaltReg<6,  "r6">;
def R7  : CobaltReg<7,  "r7">;
def R8  : CobaltReg<8,  "r8">;
def R9  : CobaltReg<9,  "r9">;
def R10 : CobaltReg<10, "r10">;
def R11 : CobaltReg<11, "r11">;
def R12 : CobaltReg<12, "r12">;
def R13 : CobaltReg<13, "r13">;
def R14 : CobaltReg<14, "r14">;
def R15 : CobaltReg<15, "r15">;
def R16 : CobaltReg<16, "r16">;
def R17 : CobaltReg<17, "r17">;
def R18 : CobaltReg<18, "r18">;
def R19 : CobaltReg<19, "r19">;
def R20 : CobaltReg<20, "r20">;
def R21 : CobaltReg<21, "r21">;
def R22 : CobaltReg<22, "r22">;
def R23 : CobaltReg<23, "r23">;
def R24 : CobaltReg<24, "r24">;
def R25 : CobaltReg<25, "r25">;
def R26 : CobaltReg<26, "r26">;
def R27 : CobaltReg<27, "r27">;
def R28 : CobaltReg<28, "r28">;
def R29 : CobaltReg<29, "r29">;
def R30 : CobaltReg<30, "r30">;
def R31 : CobaltReg<31, "r31">;

def VGPR32 : RegisterClass<"Cobalt", [i32], 32,
  (add R0, R1, R2, R3, R4, R5, R6, R7,
       R8, R9, R10, R11, R12, R13, R14, R15,
       R16, R17, R18, R19, R20, R21, R22, R23,
       R24, R25, R26, R27, R28, R29, R30, R31)>;
