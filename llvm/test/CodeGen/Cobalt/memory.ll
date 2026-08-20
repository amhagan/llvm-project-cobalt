; RUN: llc -verify-machineinstrs -mtriple=cobalt < %s | FileCheck %s

define i32 @load1(ptr %p) {
; CHECK-LABEL: load1:
; CHECK:       vld
; CHECK:       halt
  %v = load i32, ptr %p, align 4
  ret i32 %v
}

define void @store1(ptr %p, i32 %v) {
; CHECK-LABEL: store1:
; CHECK:       vst
; CHECK:       halt
  store i32 %v, ptr %p, align 4
  ret void
}

define float @load_lds_f32(ptr addrspace(3) %p) {
; CHECK-LABEL: load_lds_f32:
; CHECK:       vlds
; CHECK:       halt
  %v = load volatile float, ptr addrspace(3) %p, align 4
  ret float %v
}

define void @store_lds_f32(ptr addrspace(3) %p, ptr addrspace(8) %src) {
; CHECK-LABEL: store_lds_f32:
; CHECK:       vld
; CHECK:       vsts
; CHECK:       halt
  %v = load volatile float, ptr addrspace(8) %src, align 4
  store volatile float %v, ptr addrspace(3) %p, align 4
  ret void
}
