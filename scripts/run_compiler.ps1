# run_compiler.ps1 —— run_compiler.sh 的 PowerShell 版本(纯 Windows 环境)
# 功能:与 bash 版完全一致,使用 cmake 配置并构建 Compiler,
#       然后用它分析 sysy_test/src 下的全部 testfile*.c,
#       标准输出写入 output/<名字>_lexer.txt,标准错误写入 output/<名字>_error.txt。
# 前置条件:PATH 中存在 cmake,且存在 mingw32-make(MinGW 通常自带,位于 C:\mingw64\bin)。
# 说明:Windows 构建使用独立的 build-win 目录,避免覆盖 build 目录中 WSL 版 cmake 的缓存。
# 用法:powershell -ExecutionPolicy Bypass -File scripts\run_compiler.ps1

$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir  = Join-Path $ScriptDir 'build-win'
$OutDir    = Join-Path $ScriptDir 'output'
$Exe       = Join-Path $BuildDir 'Compiler.exe'

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw '未在 PATH 中找到 cmake。请先安装 cmake 并将其 bin 目录加入 PATH,再运行本脚本。'
}

Write-Host '===== 开始构建 Compiler ====='
& cmake -S $ScriptDir -B $BuildDir -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'cmake 配置失败' }
& cmake --build $BuildDir
if ($LASTEXITCODE -ne 0) { throw '构建失败' }
Write-Host '===== 构建完成 ====='

New-Item -ItemType Directory -Force $OutDir | Out-Null

Get-ChildItem -Path (Join-Path $ScriptDir 'sysy_test\src') -Filter 'testfile*.c' | ForEach-Object {
    $name     = $_.BaseName
    $lexerOut = Join-Path $OutDir "${name}_lexer.txt"
    $errOut   = Join-Path $OutDir "${name}_error.txt"
    Write-Host "===== 分析 $name ====="
    # 通过 cmd 完成重定向,保证输出文件的字节内容与程序实际输出一致
    cmd /c "`"$Exe`" < `"$($_.FullName)`" > `"$lexerOut`" 2> `"$errOut`"" | Out-Null
}

Write-Host "===== 全部完成,结果保存在 $OutDir ====="
