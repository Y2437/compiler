#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
SRC_DIR="$SCRIPT_DIR/sysy_test/src"
OUT_DIR="$SCRIPT_DIR/output"
EXE="$BUILD_DIR/Compiler"

echo "===== 开始构建 Compiler ====="
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR"
echo "===== 构建完成 ====="

if [ -f "$EXE.exe" ]; then
    EXE="$EXE.exe"
fi

mkdir -p "$OUT_DIR"

shopt -s nullglob
for src in "$SRC_DIR"/testfile*.c; do
    name="$(basename "$src" .c)"
    echo "===== 分析 $name ====="
    "$EXE" < "$src" > "$OUT_DIR/${name}_lexer.txt" 2> "$OUT_DIR/${name}_error.txt" || true
done

echo "===== 全部完成，结果保存在 $OUT_DIR ====="
