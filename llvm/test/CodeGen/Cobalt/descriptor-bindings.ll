; RUN: llc -verify-machineinstrs -mtriple=cobalt < %s | FileCheck %s

define i32 @load_b0(ptr addrspace(8) %p) {
; CHECK-LABEL: load_b0:
; CHECK:       vld {{r[0-9]+}}, [{{r[0-9]+}}], binding 0
  %v = load volatile i32, ptr addrspace(8) %p, align 4
  ret i32 %v
}

define i32 @load_b2(ptr addrspace(10) %p) {
; CHECK-LABEL: load_b2:
; CHECK:       vld {{r[0-9]+}}, [{{r[0-9]+}}], binding 2
  %v = load volatile i32, ptr addrspace(10) %p, align 4
  ret i32 %v
}

define void @store_b1(ptr addrspace(9) %p, i32 %v) {
; CHECK-LABEL: store_b1:
; CHECK:       vst {{r[0-9]+}}, [{{r[0-9]+}}], binding 1
  store volatile i32 %v, ptr addrspace(9) %p, align 4
  ret void
}

define void @store_b3(ptr addrspace(11) %p, i32 %v) {
; CHECK-LABEL: store_b3:
; CHECK:       vst {{r[0-9]+}}, [{{r[0-9]+}}], binding 3
  store volatile i32 %v, ptr addrspace(11) %p, align 4
  ret void
}

define i32 @atomic_b3(ptr addrspace(11) %p, i32 %v) {
; CHECK-LABEL: atomic_b3:
; CHECK:       vatomiadd {{r[0-9]+}}, [{{r[0-9]+}}], {{r[0-9]+}}, binding 3
  %old = atomicrmw add ptr addrspace(11) %p, i32 %v seq_cst
  ret i32 %old
}
