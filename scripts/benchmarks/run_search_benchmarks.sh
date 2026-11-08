#!/usr/bin/env bash
set -euo pipefail

reps=${1:-1000}

source "$(dirname "$0")/../misc/paths.sh"
OUT_DIR="$RESULTS_DIR/search"
mkdir -p "$OUT_DIR"

# FILTER="IndexSearch/large/csv/.*/partition_size:256/num_threads:32/"

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_multithreaded_32.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null

# FILTER="IndexSearch/large/csv/.*/partition_size:256/num_threads:16/"

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_multithreaded_16.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null

# FILTER="IndexSearch/large/csv/.*/partition_size:256/num_threads:8/"

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_multithreaded_8.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null

# FILTER="IndexSearch/large/csv/.*/partition_size:256/num_threads:4/"

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_multithreaded_4.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null

# FILTER="IndexSearch/large/csv/.*/partition_size:256/num_threads:2/"

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_multithreaded_2.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null

# FILTER="IndexSearch/large/csv/.*/partition_size:256/num_threads:1/"

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_multithreaded_1.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null


### Search Memory Usage Across Threads ###
# TMP="$(mktemp -d)"
# for t in 1 2 4 8 16 32; do
#     "$BENCH_BIN" \
#     --benchmark_filter="^IndexSearch/large/csv/.*/partition_size:256/num_threads:${t}/" \
#     --benchmark_out="$TMP/threads_${t}.json" \
#     --benchmark_out_format=json </dev/null
# done

# jq -s '{context: .[0].context, benchmarks: map(.benchmarks[])}' "$TMP"/*.json > "$OUT_DIR/search_large_memory_usage_threads.json"
# rm -rf "$TMP"


### Search Latency Size Comparisons ###
# FILTER="IndexSearch/small/csv/.*/partition_size:256/num_threads:1/"

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_small.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null

# FILTER="IndexSearch/medium/csv/.*/partition_size:256/num_threads:1/"

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_medium.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null

# "$BENCH_BIN" \
# --benchmark_filter="$FILTER" \
# --benchmark_out="$OUT_DIR/search_large.json" \
# --benchmark_out_format=json \
# --benchmark_report_aggregates_only=true \
# --benchmark_enable_random_interleaving=true \
# --benchmark_repetitions="$reps" </dev/null

### Search Latency File Type Comparisons ###

FILTER="IndexSearch/small/(csv|json|logfmt)/.*/partition_size:256/num_threads:1/"

"$BENCH_BIN" \
--benchmark_filter="$FILTER" \
--benchmark_out="$OUT_DIR/search_file_type_comparisons.json" \
--benchmark_out_format=json \
--benchmark_report_aggregates_only=true \
--benchmark_enable_random_interleaving=true \
--benchmark_repetitions="$reps" </dev/null

### Search Comparisons With Grep Benchmarks###
FILTER="IndexSearch/small/(csv|json|logfmt)/.*/partition_size:256/num_threads:1/"

"$BENCH_BIN" \
--benchmark_filter="$FILTER" \
--benchmark_out="$OUT_DIR/search_file_type_comparisons.json" \
--benchmark_out_format=json \
--benchmark_enable_random_interleaving=true \
--benchmark_repetitions="$reps" </dev/null
