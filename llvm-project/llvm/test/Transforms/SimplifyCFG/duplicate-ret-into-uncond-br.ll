; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -simplifycfg -simplifycfg-require-and-preserve-domtree=1 -simplifycfg-dup-ret -S | FileCheck %s

declare void @bar()
declare void @baz()

define void @foo(i1 %c) {
; CHECK-LABEL: @foo(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 [[C:%.*]], label [[TRUE:%.*]], label [[FALSE:%.*]]
; CHECK:       true:
; CHECK-NEXT:    call void @bar()
; CHECK-NEXT:    ret void
; CHECK:       false:
; CHECK-NEXT:    call void @baz()
; CHECK-NEXT:    ret void
;
entry:
  br i1 %c, label %true, label %false

true:
  call void @bar()
  br label %end

false:
  call void @baz()
  br label %end

end:
  ret void
}