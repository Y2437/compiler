#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TEST_DIR="$SCRIPT_DIR/sysy_test"

make -C "$TEST_DIR"

shopt -s nullglob
pass=0
fail=0

for input_file in "$TEST_DIR/input/"input*.txt; do
    name="$(basename "$input_file" .txt)"
    id="${name#input}"
    exe="$TEST_DIR/out/testfile$id"
    expect="$TEST_DIR/output/output$id.txt"

    if [ ! -x "$exe" ]; then
        echo "[跳过] $name: 缺少可执行文件 out/testfile$id"
        continue
    fi
    if [ ! -f "$expect" ]; then
        echo "[跳过] $name: 缺少期望输出 output/output$id.txt"
        continue
    fi

    actual="$("$exe" < <(tr -d '\r' < "$input_file") 2>/dev/null | tr -d '\r')"
    expected="$(tr -d '\r' < "$expect")"

    if [ "$actual" = "$expected" ]; then
        echo "[通过] testfile$id"
        pass=$((pass+1))
    else
        echo "[失败] testfile$id"
        fail=$((fail+1))
    fi
done

echo "=============================="
echo "通过 $pass 个, 失败 $fail 个"
[ "$fail" -eq 0 ]
