# collect_upload.ps1 —— collect_upload.sh 的 PowerShell 版本
# 功能:清空 sysy_test/upload 目录,然后收集三类文件:
#         1. sysy_test/src/*.c        -> upload/<名字>.txt
#         2. sysy_test/output/*       -> upload/
#         3. sysy_test/input/*        -> upload/
# 用法:powershell -ExecutionPolicy Bypass -File scripts\collect_upload.ps1

$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$TestDir   = Join-Path $ScriptDir 'sysy_test'
$UploadDir = Join-Path $TestDir 'upload'

if (Test-Path $UploadDir) {
    Remove-Item -Recurse -Force $UploadDir
}
New-Item -ItemType Directory -Force $UploadDir | Out-Null

Get-ChildItem -Path (Join-Path $TestDir 'src') -Filter '*.c' -ErrorAction SilentlyContinue | ForEach-Object {
    $baseName = $_.BaseName
    Copy-Item $_.FullName (Join-Path $UploadDir "$baseName.txt")
    Write-Host "已收集: src/$baseName.c -> upload/$baseName.txt"
}

foreach ($dir in @('output', 'input')) {
    Get-ChildItem -Path (Join-Path $TestDir $dir) -File -ErrorAction SilentlyContinue | ForEach-Object {
        Copy-Item $_.FullName (Join-Path $UploadDir $_.Name)
        Write-Host "已收集:   $dir/$($_.Name)"
    }
}

Write-Host "收集完成,全部文件已存放至 $UploadDir"
