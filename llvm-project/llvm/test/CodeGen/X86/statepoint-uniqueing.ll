; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -verify-machineinstrs < %s | FileCheck %s

target datalayout = "e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @use(...)
declare void @f()
declare token @llvm.experimental.gc.statepoint.p0f_isVoidf(i64, i32, void ()*, i32, i32, ...)
declare i32 addrspace(1)* @llvm.experimental.gc.relocate.p1i32(token, i32, i32) #3
declare i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token, i32, i32) #3

;; Two gc.relocates of the same input, should require only a single spill/fill
define void @test_gcrelocate_uniqueing(i32 addrspace(1)* %ptr) gc "statepoint-example" {
; CHECK-LABEL: test_gcrelocate_uniqueing:
; CHECK:       # %bb.0:
; CHECK-NEXT:    pushq %rax
; CHECK-NEXT:    .cfi_def_cfa_offset 16
; CHECK-NEXT:    movq %rdi, (%rsp)
; CHECK-NEXT:    callq f
; CHECK-NEXT:  .Ltmp0:
; CHECK-NEXT:    movq (%rsp), %rdi
; CHECK-NEXT:    movq %rdi, %rsi
; CHECK-NEXT:    xorl %eax, %eax
; CHECK-NEXT:    callq use
; CHECK-NEXT:    popq %rax
; CHECK-NEXT:    .cfi_def_cfa_offset 8
; CHECK-NEXT:    retq
  %tok = tail call token (i64, i32, void ()*, i32, i32, ...)
      @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 0, i32 0, void ()* @f, i32 0, i32 0, i32 0, i32 0) ["gc-live" (i32 addrspace(1)* %ptr, i32 addrspace(1)* %ptr), "deopt" (i32 addrspace(1)* %ptr, i32 undef)]
  %a = call i32 addrspace(1)* @llvm.experimental.gc.relocate.p1i32(token %tok, i32 0, i32 0)
  %b = call i32 addrspace(1)* @llvm.experimental.gc.relocate.p1i32(token %tok, i32 1, i32 1)
  call void (...) @use(i32 addrspace(1)* %a, i32 addrspace(1)* %b)
  ret void
}

;; Two gc.relocates of a bitcasted pointer should only require a single spill/fill
define void @test_gcptr_uniqueing(i32 addrspace(1)* %ptr) gc "statepoint-example" {
; CHECK-LABEL: test_gcptr_uniqueing:
; CHECK:       # %bb.0:
; CHECK-NEXT:    pushq %rax
; CHECK-NEXT:    .cfi_def_cfa_offset 16
; CHECK-NEXT:    movq %rdi, (%rsp)
; CHECK-NEXT:    callq f
; CHECK-NEXT:  .Ltmp1:
; CHECK-NEXT:    movq (%rsp), %rdi
; CHECK-NEXT:    movq %rdi, %rsi
; CHECK-NEXT:    xorl %eax, %eax
; CHECK-NEXT:    callq use
; CHECK-NEXT:    popq %rax
; CHECK-NEXT:    .cfi_def_cfa_offset 8
; CHECK-NEXT:    retq
  %ptr2 = bitcast i32 addrspace(1)* %ptr to i8 addrspace(1)*
  %tok = tail call token (i64, i32, void ()*, i32, i32, ...)
      @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 0, i32 0, void ()* @f, i32 0, i32 0, i32 0, i32 0) ["gc-live" (i32 addrspace(1)* %ptr, i8 addrspace(1)* %ptr2), "deopt" (i32 addrspace(1)* %ptr, i32 undef)]
  %a = call i32 addrspace(1)* @llvm.experimental.gc.relocate.p1i32(token %tok, i32 0, i32 0)
  %b = call i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token %tok, i32 1, i32 1)
  call void (...) @use(i32 addrspace(1)* %a, i8 addrspace(1)* %b)
  ret void
}

;; A GC value is not dead, and does need to be spill (but not filed) if
;; that same value is also in the deopt list.
define void @test_deopt_use(i32 addrspace(1)* %ptr) gc "statepoint-example" {
; CHECK-LABEL: test_deopt_use:
; CHECK:       # %bb.0:
; CHECK-NEXT:    pushq %rax
; CHECK-NEXT:    .cfi_def_cfa_offset 16
; CHECK-NEXT:    movq %rdi, (%rsp)
; CHECK-NEXT:    callq f
; CHECK-NEXT:  .Ltmp2:
; CHECK-NEXT:    popq %rax
; CHECK-NEXT:    .cfi_def_cfa_offset 8
; CHECK-NEXT:    retq
  tail call token (i64, i32, void ()*, i32, i32, ...)
      @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 0, i32 0, void ()* @f, i32 0, i32 0, i32 0, i32 0) ["gc-live" (i32 addrspace(1)* %ptr), "deopt" (i32 addrspace(1)* %ptr, i32 undef)]
  ret void
}

;; A GC value which is truely unused does not need to spilled or filled.
define void @test_dse(i32 addrspace(1)* %ptr) gc "statepoint-example" {
; CHECK-LABEL: test_dse:
; CHECK:       # %bb.0:
; CHECK-NEXT:    pushq %rax
; CHECK-NEXT:    .cfi_def_cfa_offset 16
; CHECK-NEXT:    callq f
; CHECK-NEXT:  .Ltmp3:
; CHECK-NEXT:    popq %rax
; CHECK-NEXT:    .cfi_def_cfa_offset 8
; CHECK-NEXT:    retq
  tail call token (i64, i32, void ()*, i32, i32, ...)
      @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 0, i32 0, void ()* @f, i32 0, i32 0, i32 0, i32 0) ["gc-live" (i32 addrspace(1)* %ptr)]
  ret void
}