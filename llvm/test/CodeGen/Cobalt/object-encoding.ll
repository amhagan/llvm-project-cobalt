; RUN: llc -verify-machineinstrs -mtriple=cobalt -filetype=obj < %s -o %t.o
; RUN: llvm-readobj --hex-dump=.text %t.o | FileCheck %s

define i32 @add2(i32 %a, i32 %b) {
  %v = add i32 %a, %b
  ret i32 %v
}

; CHECK: Hex dump of section '.text':
; CHECK-NEXT: 0x00000000 00400004 000000fc
