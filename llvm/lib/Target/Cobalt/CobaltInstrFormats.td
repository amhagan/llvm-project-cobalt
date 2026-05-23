//===-- CobaltInstrFormats.td - Cobalt instruction formats -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class CobaltInst<bits<8> Opcode, dag Outs, dag Ins, string Asm,
                 list<dag> Pat = []> : Instruction {
  field bits<32> Inst;

  let Namespace = "Cobalt";
  let Size = 4;
  let OutOperandList = Outs;
  let InOperandList = Ins;
  let AsmString = Asm;
  let Pattern = Pat;

  let Inst{31-24} = Opcode;
  let Inst{23-0} = 0;
}

class CobaltPseudo<dag Outs, dag Ins, string Asm, list<dag> Pat = []>
    : CobaltInst<0xff, Outs, Ins, Asm, Pat> {
  let isPseudo = 1;
}
