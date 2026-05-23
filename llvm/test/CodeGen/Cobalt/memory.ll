; RUN: llc -verify-machineinstrs -mtriple=cobalt < %s | FileCheck %s

define i32 @load1(ptr %p) {
; CHECK-LABEL: load1:
; CHECK:       vld r0, [r0]
; CHECK:       halt
  %v = load i32, ptr %p, align 4
  ret i32 %v
}

define void @store1(ptr %p, i32 %v) {
; CHECK-LABEL: store1:
; CHECK:       vst r1, [r0]
; CHECK:       halt
  store i32 %v, ptr %p, align 4
  ret void
}
