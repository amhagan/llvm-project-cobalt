; RUN: llc -verify-machineinstrs -mtriple=cobalt < %s | FileCheck %s

; Cobalt intentionally expands switches into direct conditional branches until
; the ISA and backend have real indirect-branch/jump-table support.
define void @dense_switch(ptr %out, i32 %selector) {
; CHECK-LABEL: dense_switch:
; CHECK-NOT:   JTI
; CHECK:       vbrnz
; CHECK:       vbr
; CHECK:       halt
entry:
  switch i32 %selector, label %default [
    i32 0, label %case0
    i32 1, label %case1
    i32 2, label %case2
    i32 3, label %case3
    i32 4, label %case4
    i32 5, label %case5
    i32 6, label %case6
    i32 7, label %case7
  ]

case0:
  store i32 17, ptr %out, align 4
  ret void
case1:
  store i32 29, ptr %out, align 4
  ret void
case2:
  store i32 43, ptr %out, align 4
  ret void
case3:
  store i32 61, ptr %out, align 4
  ret void
case4:
  store i32 79, ptr %out, align 4
  ret void
case5:
  store i32 101, ptr %out, align 4
  ret void
case6:
  store i32 127, ptr %out, align 4
  ret void
case7:
  store i32 157, ptr %out, align 4
  ret void
default:
  store i32 211, ptr %out, align 4
  ret void
}
