#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/../misc/paths.sh"
OUT_DIR="$RESULTS_DIR/creation"
mkdir -p "$OUT_DIR"

"$BENCH_BIN" \
--benchmark_filter='IndexCreate/small/[^/]+/.*/partition_size:128/num_threads:1$' \
--benchmark_out="$OUT_DIR/creation.json" \
--benchmark_out_format=json \
--benchmark_report_aggregates_only=true 