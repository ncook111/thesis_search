#!/usr/bin/env bash
set -euo pipefail

reps=${1:-1000}

source "$(dirname "$0")/../misc/paths.sh"
OUT_DIR="$RESULTS_DIR/search"
mkdir -p "$OUT_DIR"

"$BENCH_BIN" \
--benchmark_filter=IndexSearch/small/csv \
--benchmark_out="$OUT_DIR/search.json" \
--benchmark_out_format=json \
--benchmark_report_aggregates_only=true \
--benchmark_enable_random_interleaving=true \
--benchmark_repetitions="$reps"