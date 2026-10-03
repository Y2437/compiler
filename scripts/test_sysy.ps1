# test_sysy.ps1 —— test_sysy.sh 的 PowerShell 版本(纯 Windows 环境)
# 功能:第一步用 MinGW 的 gcc 构建 sysy_test/src 下的参考答案程序
#         (编译参数与 sysy_test/Makefile 一致,产物为 out/testfileN.exe);
#       第二步对每一个 input/inputN.txt 运行 out/testfileN.exe,
#       与 output/outputN.txt 逐字比对(忽略 \r),报告通过与失败的数量。
# 用法:powershell -ExecutionPolicy Bypass -File scripts\test_sysy.ps1

$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$TestDir   = Join-Path $ScriptDir 'sysy_test'
$SrcDir    = Join-Path $TestDir 'src'
$IncDir    = Join-Path $TestDir 'include'
$OutDir    = Join-Path $TestDir 'out'
$InputDir  = Join-Path $TestDir 'input'
$ExpectDir = Join-Path $TestDir 'output'

New-Item -ItemType Directory -Force $OutDir | Out-Null

# ===== 第一步:构建参考答案程序(等价于 sysy_test/Makefile) =====
$libsysyC   = Join-Path $IncDir 'libsysy.c'
$libsysyH   = Join-Path $IncDir 'libsysy.h'
$libsysyTime = (Get-Item $libsysyC).LastWriteTime

Get-ChildItem -Path $SrcDir -Filter 'testfile*.c' | ForEach-Object {
    $name = $_.BaseName
    $exe  = Join-Path $OutDir "$name.exe"
    $needBuild = (-not (Test-Path $exe)) -or
                 ($_.LastWriteTime -gt (Get-Item $exe).LastWriteTime) -or
                 ($libsysyTime -gt (Get-Item $exe).LastWriteTime)
    if ($needBuild) {
        Write-Host "===== 构建 $name.exe ====="
        & gcc -O2 -Wall "-I$IncDir" -include $libsysyH -x c $_.FullName -x none $libsysyC -o $exe
        if ($LASTEXITCODE -ne 0) { throw "gcc 构建 $name 失败" }
    }
}

# ===== 第二步:逐个输入样例,比对输出 =====
$pass = 0
$fail = 0

Get-ChildItem -Path $InputDir -Filter 'input*.txt' | ForEach-Object {
    $name   = $_.BaseName                    # 例如 input1
    $id     = $name -replace '^input', ''    # 例如 1
    $exe    = Join-Path $OutDir "testfile$id.exe"
    $expect = Join-Path $ExpectDir "output$id.txt"

    if (-not (Test-Path $exe)) {
        Write-Host "[跳过] ${name}: 缺少可执行文件 out/testfile$id.exe"
        return
    }
    if (-not (Test-Path $expect)) {
        Write-Host "[跳过] ${name}: 缺少期望输出 output/output$id.txt"
        return
    }

    $tmp = Join-Path $env:TEMP "sysy_actual_$id.txt"
    cmd /c "`"$exe`" < `"$($_.FullName)`" > `"$tmp`" 2>nul"
    $actual = Get-Content $tmp -Raw
    Remove-Item $tmp -Force
    if (-not $actual) { $actual = '' }
    $actual = ($actual -replace "`r", '').TrimEnd("`n")

    $expected = Get-Content $expect -Raw
    if (-not $expected) { $expected = '' }
    $expected = ($expected -replace "`r", '').TrimEnd("`n")

    if ($actual -ceq $expected) {
        Write-Host "[通过] testfile$id"
        $script:pass++
    } else {
        Write-Host "[失败] testfile$id"
        $script:fail++
    }
}

Write-Host '=============================='
Write-Host "通过 $pass 个, 失败 $fail 个"
if ($fail -ne 0) { exit 1 }
