; ModuleID = 'lib/libsysy.c'
source_filename = "lib/libsysy.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [11 x i8] c"value >= 0\00", align 1
@.str.1 = private unnamed_addr constant [14 x i8] c"lib/libsysy.c\00", align 1
@__PRETTY_FUNCTION__.get_char = private unnamed_addr constant [16 x i8] c"char get_char()\00", align 1
@stdin = external global ptr, align 8
@stderr = external global ptr, align 8
@.str.2 = private unnamed_addr constant [30 x i8] c"libsys: get_int: EOF reached\0A\00", align 1
@.str.3 = private unnamed_addr constant [29 x i8] c"libsys: get_int: empty line\0A\00", align 1
@.str.4 = private unnamed_addr constant [33 x i8] c"libsys: get_int: invalid digits\0A\00", align 1
@.str.5 = private unnamed_addr constant [46 x i8] c"libsys: get_int: invalid trailing characters\0A\00", align 1
@.str.6 = private unnamed_addr constant [39 x i8] c"libsys: get_int: integer out of range\0A\00", align 1
@.str.7 = private unnamed_addr constant [37 x i8] c"libsys: get_string: input error: %s\0A\00", align 1
@.str.8 = private unnamed_addr constant [55 x i8] c"libsys: get_string: failed to restore input character\0A\00", align 1
@.str.9 = private unnamed_addr constant [33 x i8] c"libsys: get_string: EOF reached\0A\00", align 1
@.str.10 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.11 = private unnamed_addr constant [3 x i8] c"%c\00", align 1
@.str.12 = private unnamed_addr constant [3 x i8] c"%s\00", align 1

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local signext i8 @get_char() #0 {
  %1 = alloca i32, align 4
  %2 = call i32 @getchar()
  store i32 %2, ptr %1, align 4
  %3 = load i32, ptr %1, align 4
  %4 = icmp sge i32 %3, 0
  br i1 %4, label %5, label %6

5:                                                ; preds = %0
  br label %7

6:                                                ; preds = %0
  call void @__assert_fail(ptr noundef @.str, ptr noundef @.str.1, i32 noundef 16, ptr noundef @__PRETTY_FUNCTION__.get_char) #5
  unreachable

7:                                                ; preds = %5
  %8 = load i32, ptr %1, align 4
  %9 = trunc i32 %8 to i8
  ret i8 %9
}

declare i32 @getchar() #1

; Function Attrs: cold noreturn nounwind
declare void @__assert_fail(ptr noundef, ptr noundef, i32 noundef, ptr noundef) #2

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local i32 @get_int() #0 {
  %1 = alloca [512 x i8], align 16
  %2 = alloca ptr, align 8
  %3 = alloca ptr, align 8
  %4 = alloca i64, align 8
  %5 = getelementptr inbounds [512 x i8], ptr %1, i64 0, i64 0
  %6 = load ptr, ptr @stdin, align 8
  %7 = call ptr @fgets(ptr noundef %5, i32 noundef 512, ptr noundef %6)
  %8 = icmp eq ptr %7, null
  br i1 %8, label %9, label %12

9:                                                ; preds = %0
  %10 = load ptr, ptr @stderr, align 8
  %11 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %10, ptr noundef @.str.2) #6
  call void @abort() #5
  unreachable

12:                                               ; preds = %0
  %13 = getelementptr inbounds [512 x i8], ptr %1, i64 0, i64 0
  store ptr %13, ptr %2, align 8
  br label %14

14:                                               ; preds = %26, %12
  %15 = call ptr @__ctype_b_loc() #7
  %16 = load ptr, ptr %15, align 8
  %17 = load ptr, ptr %2, align 8
  %18 = load i8, ptr %17, align 1
  %19 = sext i8 %18 to i32
  %20 = sext i32 %19 to i64
  %21 = getelementptr inbounds i16, ptr %16, i64 %20
  %22 = load i16, ptr %21, align 2
  %23 = zext i16 %22 to i32
  %24 = and i32 %23, 8192
  %25 = icmp ne i32 %24, 0
  br i1 %25, label %26, label %29

26:                                               ; preds = %14
  %27 = load ptr, ptr %2, align 8
  %28 = getelementptr inbounds nuw i8, ptr %27, i32 1
  store ptr %28, ptr %2, align 8
  br label %14, !llvm.loop !6

29:                                               ; preds = %14
  %30 = load ptr, ptr %2, align 8
  %31 = load i8, ptr %30, align 1
  %32 = sext i8 %31 to i32
  %33 = icmp eq i32 %32, 0
  br i1 %33, label %34, label %37

34:                                               ; preds = %29
  %35 = load ptr, ptr @stderr, align 8
  %36 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %35, ptr noundef @.str.3) #6
  call void @abort() #5
  unreachable

37:                                               ; preds = %29
  %38 = call ptr @__errno_location() #7
  store i32 0, ptr %38, align 4
  store ptr null, ptr %3, align 8
  %39 = load ptr, ptr %2, align 8
  %40 = call i64 @strtol(ptr noundef %39, ptr noundef %3, i32 noundef 10) #6
  store i64 %40, ptr %4, align 8
  %41 = load ptr, ptr %2, align 8
  %42 = load ptr, ptr %3, align 8
  %43 = icmp eq ptr %41, %42
  br i1 %43, label %44, label %47

44:                                               ; preds = %37
  %45 = load ptr, ptr @stderr, align 8
  %46 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %45, ptr noundef @.str.4) #6
  call void @abort() #5
  unreachable

47:                                               ; preds = %37
  br label %48

48:                                               ; preds = %60, %47
  %49 = call ptr @__ctype_b_loc() #7
  %50 = load ptr, ptr %49, align 8
  %51 = load ptr, ptr %3, align 8
  %52 = load i8, ptr %51, align 1
  %53 = sext i8 %52 to i32
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds i16, ptr %50, i64 %54
  %56 = load i16, ptr %55, align 2
  %57 = zext i16 %56 to i32
  %58 = and i32 %57, 8192
  %59 = icmp ne i32 %58, 0
  br i1 %59, label %60, label %63

60:                                               ; preds = %48
  %61 = load ptr, ptr %3, align 8
  %62 = getelementptr inbounds nuw i8, ptr %61, i32 1
  store ptr %62, ptr %3, align 8
  br label %48, !llvm.loop !8

63:                                               ; preds = %48
  %64 = load ptr, ptr %3, align 8
  %65 = load i8, ptr %64, align 1
  %66 = sext i8 %65 to i32
  %67 = icmp ne i32 %66, 0
  br i1 %67, label %68, label %71

68:                                               ; preds = %63
  %69 = load ptr, ptr @stderr, align 8
  %70 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %69, ptr noundef @.str.5) #6
  call void @abort() #5
  unreachable

71:                                               ; preds = %63
  %72 = call ptr @__errno_location() #7
  %73 = load i32, ptr %72, align 4
  %74 = icmp eq i32 %73, 34
  br i1 %74, label %81, label %75

75:                                               ; preds = %71
  %76 = load i64, ptr %4, align 8
  %77 = icmp slt i64 %76, -2147483648
  br i1 %77, label %81, label %78

78:                                               ; preds = %75
  %79 = load i64, ptr %4, align 8
  %80 = icmp sgt i64 %79, 2147483647
  br i1 %80, label %81, label %84

81:                                               ; preds = %78, %75, %71
  %82 = load ptr, ptr @stderr, align 8
  %83 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %82, ptr noundef @.str.6) #6
  call void @abort() #5
  unreachable

84:                                               ; preds = %78
  %85 = load i64, ptr %4, align 8
  %86 = trunc i64 %85 to i32
  ret i32 %86
}

declare ptr @fgets(ptr noundef, i32 noundef, ptr noundef) #1

; Function Attrs: nounwind
declare i32 @fprintf(ptr noundef, ptr noundef, ...) #3

; Function Attrs: cold noreturn nounwind
declare void @abort() #2

; Function Attrs: nounwind willreturn memory(none)
declare ptr @__ctype_b_loc() #4

; Function Attrs: nounwind willreturn memory(none)
declare ptr @__errno_location() #4

; Function Attrs: nounwind
declare i64 @strtol(ptr noundef, ptr noundef, i32 noundef) #3

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local void @get_string(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  store i32 0, ptr %5, align 4
  %10 = load i32, ptr %4, align 4
  %11 = icmp sgt i32 %10, 0
  br i1 %11, label %12, label %15

12:                                               ; preds = %2
  %13 = load i32, ptr %4, align 4
  %14 = sub nsw i32 %13, 1
  br label %16

15:                                               ; preds = %2
  br label %16

16:                                               ; preds = %15, %12
  %17 = phi i32 [ %14, %12 ], [ 0, %15 ]
  store i32 %17, ptr %6, align 4
  store i32 0, ptr %7, align 4
  br label %18

18:                                               ; preds = %71, %16
  %19 = load ptr, ptr @stdin, align 8
  %20 = call i32 @fgetc(ptr noundef %19)
  store i32 %20, ptr %8, align 4
  %21 = icmp ne i32 %20, -1
  br i1 %21, label %22, label %72

22:                                               ; preds = %18
  store i32 1, ptr %7, align 4
  %23 = load i32, ptr %8, align 4
  %24 = icmp eq i32 %23, 10
  br i1 %24, label %25, label %26

25:                                               ; preds = %22
  br label %72

26:                                               ; preds = %22
  %27 = load i32, ptr %8, align 4
  %28 = icmp eq i32 %27, 13
  br i1 %28, label %29, label %58

29:                                               ; preds = %26
  %30 = load ptr, ptr @stdin, align 8
  %31 = call i32 @fgetc(ptr noundef %30)
  store i32 %31, ptr %9, align 4
  %32 = load i32, ptr %9, align 4
  %33 = icmp eq i32 %32, -1
  br i1 %33, label %34, label %45

34:                                               ; preds = %29
  %35 = load ptr, ptr @stdin, align 8
  %36 = call i32 @ferror(ptr noundef %35) #6
  %37 = icmp ne i32 %36, 0
  br i1 %37, label %38, label %44

38:                                               ; preds = %34
  %39 = load ptr, ptr @stderr, align 8
  %40 = call ptr @__errno_location() #7
  %41 = load i32, ptr %40, align 4
  %42 = call ptr @strerror(i32 noundef %41) #6
  %43 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %39, ptr noundef @.str.7, ptr noundef %42) #6
  call void @abort() #5
  unreachable

44:                                               ; preds = %34
  br label %57

45:                                               ; preds = %29
  %46 = load i32, ptr %9, align 4
  %47 = icmp ne i32 %46, 10
  br i1 %47, label %48, label %56

48:                                               ; preds = %45
  %49 = load i32, ptr %9, align 4
  %50 = load ptr, ptr @stdin, align 8
  %51 = call i32 @ungetc(i32 noundef %49, ptr noundef %50)
  %52 = icmp eq i32 %51, -1
  br i1 %52, label %53, label %56

53:                                               ; preds = %48
  %54 = load ptr, ptr @stderr, align 8
  %55 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %54, ptr noundef @.str.8) #6
  call void @abort() #5
  unreachable

56:                                               ; preds = %48, %45
  br label %57

57:                                               ; preds = %56, %44
  br label %72

58:                                               ; preds = %26
  %59 = load i32, ptr %5, align 4
  %60 = load i32, ptr %6, align 4
  %61 = icmp slt i32 %59, %60
  br i1 %61, label %62, label %71

62:                                               ; preds = %58
  %63 = load i32, ptr %8, align 4
  %64 = trunc i32 %63 to i8
  %65 = load ptr, ptr %3, align 8
  %66 = load i32, ptr %5, align 4
  %67 = sext i32 %66 to i64
  %68 = getelementptr inbounds i8, ptr %65, i64 %67
  store i8 %64, ptr %68, align 1
  %69 = load i32, ptr %5, align 4
  %70 = add nsw i32 %69, 1
  store i32 %70, ptr %5, align 4
  br label %71

71:                                               ; preds = %62, %58
  br label %18, !llvm.loop !9

72:                                               ; preds = %57, %25, %18
  %73 = load i32, ptr %8, align 4
  %74 = icmp eq i32 %73, -1
  br i1 %74, label %75, label %92

75:                                               ; preds = %72
  %76 = load ptr, ptr @stdin, align 8
  %77 = call i32 @ferror(ptr noundef %76) #6
  %78 = icmp ne i32 %77, 0
  br i1 %78, label %79, label %85

79:                                               ; preds = %75
  %80 = load ptr, ptr @stderr, align 8
  %81 = call ptr @__errno_location() #7
  %82 = load i32, ptr %81, align 4
  %83 = call ptr @strerror(i32 noundef %82) #6
  %84 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %80, ptr noundef @.str.7, ptr noundef %83) #6
  call void @abort() #5
  unreachable

85:                                               ; preds = %75
  %86 = load i32, ptr %7, align 4
  %87 = icmp ne i32 %86, 0
  br i1 %87, label %91, label %88

88:                                               ; preds = %85
  %89 = load ptr, ptr @stderr, align 8
  %90 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %89, ptr noundef @.str.9) #6
  call void @abort() #5
  unreachable

91:                                               ; preds = %85
  br label %92

92:                                               ; preds = %91, %72
  %93 = load i32, ptr %4, align 4
  %94 = icmp sle i32 %93, 0
  br i1 %94, label %95, label %96

95:                                               ; preds = %92
  br label %112

96:                                               ; preds = %92
  %97 = load i32, ptr %5, align 4
  %98 = load i32, ptr %6, align 4
  %99 = icmp slt i32 %97, %98
  br i1 %99, label %100, label %107

100:                                              ; preds = %96
  %101 = load ptr, ptr %3, align 8
  %102 = load i32, ptr %5, align 4
  %103 = sext i32 %102 to i64
  %104 = getelementptr inbounds i8, ptr %101, i64 %103
  store i8 10, ptr %104, align 1
  %105 = load i32, ptr %5, align 4
  %106 = add nsw i32 %105, 1
  store i32 %106, ptr %5, align 4
  br label %107

107:                                              ; preds = %100, %96
  %108 = load ptr, ptr %3, align 8
  %109 = load i32, ptr %5, align 4
  %110 = sext i32 %109 to i64
  %111 = getelementptr inbounds i8, ptr %108, i64 %110
  store i8 0, ptr %111, align 1
  br label %112

112:                                              ; preds = %107, %95
  ret void
}

declare i32 @fgetc(ptr noundef) #1

; Function Attrs: nounwind
declare i32 @ferror(ptr noundef) #3

; Function Attrs: nounwind
declare ptr @strerror(i32 noundef) #3

declare i32 @ungetc(i32 noundef, ptr noundef) #1

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local void @put_int(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.10, i32 noundef %3)
  ret void
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local void @put_char(i8 noundef signext %0) #0 {
  %2 = alloca i8, align 1
  store i8 %0, ptr %2, align 1
  %3 = load i8, ptr %2, align 1
  %4 = sext i8 %3 to i32
  %5 = call i32 (ptr, ...) @printf(ptr noundef @.str.11, i32 noundef %4)
  ret void
}

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local void @put_string(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.12, ptr noundef %3)
  ret void
}

attributes #0 = { noinline nounwind optnone sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { cold noreturn nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind willreturn memory(none) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { cold noreturn nounwind }
attributes #6 = { nounwind }
attributes #7 = { nounwind willreturn memory(none) }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 22.1.8"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
