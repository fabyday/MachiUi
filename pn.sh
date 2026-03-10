#!/bin/bash

TARGET_DIR="$(cd "$(dirname "$0")" && pwd)/Source/JavaScript"

# 디렉토리 존재 여부 확인 (선택 사항이지만 권장)
if [ -d "$TARGET_DIR" ]; then
    cd "$TARGET_DIR" || exit
    echo "Target Directory: $(pwd)"
    
    # 전달받은 모든 인자($*)를 pnpm에 전달
    # call pnpm %*와 동일한 역할
    pnpm "$@"
else
    echo "Error: Directory $TARGET_DIR not found."
    exit 1
fi