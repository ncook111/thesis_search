#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/../misc/paths.sh"
OUT_DIR="$RESULTS_DIR/creation"
mkdir -p "$OUT_DIR"

# ### Creation benchmarking comparing file types ###
"$BENCH_BIN" \
--benchmark_filter='IndexCreate/small/[^/]+/.*/partition_size:256/num_threads:1/' \
--benchmark_out="$OUT_DIR/creation_file_types.json" \
--benchmark_out_format=json

# ### Creation benchmarking comparing partition sizes and memory usage ###
# P_SIZES="8|16|32|64|128|256|512|1024"
# FILTER="IndexCreate/medium/csv/.*/partition_size:($P_SIZES)/num_threads:1/"
# TMP=$(mktemp -d)

# "$BENCH_BIN" --benchmark_list_tests --benchmark_filter="$FILTER" | while read -r name; do
#     "$BENCH_BIN" \
#     --benchmark_filter="^${name}\$" \
#     --benchmark_out="$TMP/$(echo "$name" | tr '/:' '__').json" \
#     --benchmark_out_format=json </dev/null
# done

# jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/creation_partition.json"
# rm -rf "$TMP"

### Creation benchmarking comparing threads and memory usage###
# FILTER="IndexCreate/large/csv/.*/partition_size:256/num_threads:1/"
# TMP=$(mktemp -d)

# "$BENCH_BIN" --benchmark_list_tests --benchmark_filter="$FILTER" | while read -r name; do
#     "$BENCH_BIN" \
#     --benchmark_filter="^${name}\$" \
#     --benchmark_out="$TMP/$(echo "$name" | tr '/:' '__').json" \
#     --benchmark_out_format=json </dev/null
# done

# jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/creation_multithreaded_1.json"
# rm -rf "$TMP"

# FILTER="IndexCreate/large/csv/.*/partition_size:256/num_threads:2/"
# TMP=$(mktemp -d)

# "$BENCH_BIN" --benchmark_list_tests --benchmark_filter="$FILTER" | while read -r name; do
#     "$BENCH_BIN" \
#     --benchmark_filter="^${name}\$" \
#     --benchmark_out="$TMP/$(echo "$name" | tr '/:' '__').json" \
#     --benchmark_out_format=json </dev/null
# done

# jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/creation_multithreaded_2.json"
# rm -rf "$TMP"

# FILTER="IndexCreate/large/csv/.*/partition_size:256/num_threads:4/"
# TMP=$(mktemp -d)

# "$BENCH_BIN" --benchmark_list_tests --benchmark_filter="$FILTER" | while read -r name; do
#     "$BENCH_BIN" \
#     --benchmark_filter="^${name}\$" \
#     --benchmark_out="$TMP/$(echo "$name" | tr '/:' '__').json" \
#     --benchmark_out_format=json </dev/null
# done

# jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/creation_multithreaded_4.json"
# rm -rf "$TMP"

# FILTER="IndexCreate/large/csv/.*/partition_size:256/num_threads:8/"
# TMP=$(mktemp -d)

# "$BENCH_BIN" --benchmark_list_tests --benchmark_filter="$FILTER" | while read -r name; do
#     "$BENCH_BIN" \
#     --benchmark_filter="^${name}\$" \
#     --benchmark_out="$TMP/$(echo "$name" | tr '/:' '__').json" \
#     --benchmark_out_format=json </dev/null
# done

# jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/creation_multithreaded_8.json"
# rm -rf "$TMP"

# FILTER="IndexCreate/large/csv/.*/partition_size:256/num_threads:16/"
# TMP=$(mktemp -d)

# "$BENCH_BIN" --benchmark_list_tests --benchmark_filter="$FILTER" | while read -r name; do
#     "$BENCH_BIN" \
#     --benchmark_filter="^${name}\$" \
#     --benchmark_out="$TMP/$(echo "$name" | tr '/:' '__').json" \
#     --benchmark_out_format=json </dev/null
# done

# jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/creation_multithreaded_16.json"
# rm -rf "$TMP"

# FILTER="IndexCreate/large/csv/.*/partition_size:256/num_threads:32/"
# TMP=$(mktemp -d)

# "$BENCH_BIN" --benchmark_list_tests --benchmark_filter="$FILTER" | while read -r name; do
#     "$BENCH_BIN" \
#     --benchmark_filter="^${name}\$" \
#     --benchmark_out="$TMP/$(echo "$name" | tr '/:' '__').json" \
#     --benchmark_out_format=json </dev/null
# done

# jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/creation_multithreaded_32.json"
# rm -rf "$TMP"