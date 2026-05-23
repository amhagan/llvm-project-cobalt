; RUN: llc -verify-machineinstrs -mtriple=cobalt < %s | FileCheck %s

define i32 @add2(i32 %a, i32 %b) {
; CHECK-LABEL: add2:
; CHECK:       vadd r0, r0, r1
; CHECK:       halt
  %v = add i32 %a, %b
  ret i32 %v
}

define i32 @sub2(i32 %a, i32 %b) {
; CHECK-LABEL: sub2:
; CHECK:       vsub r0, r0, r1
; CHECK:       halt
  %v = sub i32 %a, %b
  ret i32 %v
}

define i32 @mul2(i32 %a, i32 %b) {
; CHECK-LABEL: mul2:
; CHECK:       vmul r0, r0, r1
; CHECK:       halt
  %v = mul i32 %a, %b
  ret i32 %v
}

define i32 @and2(i32 %a, i32 %b) {
; CHECK-LABEL: and2:
; CHECK:       vand r0, r0, r1
; CHECK:       halt
  %v = and i32 %a, %b
  ret i32 %v
}

define i32 @or2(i32 %a, i32 %b) {
; CHECK-LABEL: or2:
; CHECK:       vor r0, r0, r1
; CHECK:       halt
  %v = or i32 %a, %b
  ret i32 %v
}

define i32 @xor2(i32 %a, i32 %b) {
; CHECK-LABEL: xor2:
; CHECK:       vxor r0, r0, r1
; CHECK:       halt
  %v = xor i32 %a, %b
  ret i32 %v
}
