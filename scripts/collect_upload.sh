#!/bin/bash
set -e

shopt -s nullglob

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TEST_DIR="$SCRIPT_DIR/sysy_test"
UPLOAD_DIR="$TEST_DIR/upload"


rm -rf "$UPLOAD_DIR"
mkdir -p "$UPLOAD_DIR"

for src_file in "$TEST_DIR/src/"*.c; do
    base_name="$(basename "$src_file" .c)"
    cp "$src_file" "$UPLOAD_DIR/$base_name.txt"
    echo "已收集: src/$base_name.c -> upload/$base_name.txt"
done


for dir in output input; do
    for file in "$TEST_DIR/$dir/"*; do
        [ -f "$file" ] || continue
        cp "$file" "$UPLOAD_DIR/$(basename "$file")"
        echo "已收集:   $dir/$(basename "$file")"
    done
done

echo "收集完成，全部文件已存放至 $UPLOAD_DIR"
