#!/usr/bin/env bash
set -euo pipefail

reps=${1:-1000}

source "$(dirname "$0")/../misc/paths.sh"
OUT_DIR="$RESULTS_DIR/search"
mkdir -p "$OUT_DIR"

FILTER="IndexSearch/small/csv/.*/partition_size:256/num_threads:4/"
TMP=$(mktemp -d)

    "$BENCH_BIN" \
    --benchmark_filter="^${name}\$" \
    --benchmark_out="$OUT_DIR/search.json" \
    --benchmark_out_format=json \
    --benchmark_repetitions="$reps" </dev/null