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

def VGPR32 : RegisterClass<"Cobalt", [i32], 32,
  (add R0, R1, R2, R3, R4, R5, R6, R7,
       R8, R9, R10, R11, R12, R13, R14, R15)>;
