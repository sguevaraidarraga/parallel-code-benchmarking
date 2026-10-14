#!/usr/bin/env bash
# Prepared for later use; run explicitly from Linux or WSL.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
INPUT_FILES=(input512x512.bmp input1024x1024.bmp input2048x2048.bmp input4096x4096.bmp)
KERNEL_SIZES=(3 5 7 11)
THREAD_COUNTS=(1 2 4 8 16)
for input in "${INPUT_FILES[@]}"; do
    if [[ ! -f "$input" ]]; then
        printf 'Missing required input: %s\n' "$input" >&2
        exit 1
    fi
done
# New directory for each invocation preserves previous results.
mkdir -p benchmark_results
OUTPUT_DIR=$(mktemp -d benchmark_results/run-XXXXXX)
mkdir -p "$OUTPUT_DIR/images"
g++ -std=c++11 -O3 -fopenmp -Wall -Wextra convolve.cpp -o "$OUTPUT_DIR/convolve"
for input in "${INPUT_FILES[@]}"; do
    for kernel in "${KERNEL_SIZES[@]}"; do
        for threads in "${THREAD_COUNTS[@]}"; do
            name="${input%.bmp}_k${kernel}_t${threads}"
            printf 'Processing %s\n' "$name"
            "$OUTPUT_DIR/convolve" "$input" "$OUTPUT_DIR/images/$name.bmp" \
                "$threads" "$kernel" --benchmark
        done
    done
done
printf 'Processed images: %s\n' "$OUTPUT_DIR/images"
