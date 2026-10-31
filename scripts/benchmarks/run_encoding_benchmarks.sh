#!/usr/bin/env bash
set -euo pipefail

reps=${1:-1000}

source "$(dirname "$0")/../misc/paths.sh"
OUT_DIR="$RESULTS_DIR/encodings"
mkdir -p "$OUT_DIR"

### Benchmark comparing schemes, capturing tail latencies ###
# Compression Encoding
"$BENCH_BIN" \
--benchmark_filter=cenc \
--benchmark_out="$OUT_DIR/compression_encoding.json" \
--benchmark_out_format=json \
--benchmark_report_aggregates_only=true \
--benchmark_enable_random_interleaving=true \
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

### Benchmark comparing schemes, capturing memory usage ###
FILTER="(cenc|cdec|denc|ddec).*/1000000/"
TMP=$(mktemp -d)

"$BENCH_BIN" --benchmark_list_tests --benchmark_filter="$FILTER" | while read -r name; do
    "$BENCH_BIN" \
    --benchmark_filter="^${name}\$" \
    --benchmark_out="$TMP/$(echo "$name" | tr '/:' '__').json" \
    --benchmark_out_format=json \
    --benchmark_repetitions=1 </dev/null
done

jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/compression_memory_usage.json"
rm -rf "$TMP"