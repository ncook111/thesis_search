#!/usr/bin/env bash
set -euo pipefail

reps=${1:-1000}

source "$(dirname "$0")/../misc/paths.sh"
OUT_DIR="$RESULTS_DIR/encodings_warm"
mkdir -p "$OUT_DIR"

# # ### Benchmark comparing schemes, capturing tail latencies ###
# Compression Encoding
"$BENCH_BIN" \
--benchmark_filter=cenc \
--benchmark_out="a.json" \
--benchmark_out_format=json \
--benchmark_report_aggregates_only=true \
--benchmark_repetitions="$reps"

# Compression Decoding
"$BENCH_BIN" \
--benchmark_filter=cdec \
--benchmark_out="$OUT_DIR/compression_decoding.json" \
--benchmark_out_format=json \
--benchmark_report_aggregates_only=true \
--benchmark_enable_random_interleaving=true \
--benchmark_repetitions="$reps"

# Delta Encoding
"$BENCH_BIN" \
--benchmark_filter=denc \
--benchmark_out="$OUT_DIR/delta_encoding.json" \
--benchmark_out_format=json \
--benchmark_report_aggregates_only=true \
--benchmark_enable_random_interleaving=true \
--benchmark_repetitions="$reps"

# Delta Decoding
"$BENCH_BIN" \
--benchmark_filter=ddec \
--benchmark_out="$OUT_DIR/delta_decoding.json" \
--benchmark_out_format=json \
--benchmark_report_aggregates_only=true \
--benchmark_enable_random_interleaving=true \
--benchmark_repetitions="$reps"