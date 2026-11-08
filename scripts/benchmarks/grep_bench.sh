#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/../misc/paths.sh"
CREATION_OUT_DIR="$RESULTS_DIR/creation_lz4_gz"
SEARCH_OUT_DIR="$RESULTS_DIR/creation_lz4_gz"

mkdir -p "$CREATION_OUT_DIR"
mkdir -p "$SEARCH_OUT_DIR"

GTIME=/usr/bin/time

CREATION="$CREATION_OUT_DIR/creation_lz4_gz.json"
TIMES=$(mktemp)
TIMES2=$(mktemp)
BASE_DIR=$(mktemp -d)
trap 'rm -f "$TIMES" "$TIMES2"; rm -rf "$BASE_DIR"' EXIT

SEARCH="$SEARCH_OUT_DIR/search_lz4_gz.json"
sep=""

: > "$BASE_DIR/empty"
lz4 -q -f "$BASE_DIR/empty" "$BASE_DIR/empty.lz4"
gzip -c "$BASE_DIR/empty" > "$BASE_DIR/empty.gzip"

setup_compress_data() {
  mkdir -p "$BENCH_DATA"/tmp_compressed/{csv,json,logfmt}/small_data/data_{1..4}

  printf '{\n"benchmarks": [\n' >> "$CREATION"
  sep=""

  # csv
  compress_loop "small" "csv" "lz4"
  compress_loop "small" "csv" "gzip"

  echo "Compressed CSV"

  # json
  compress_loop "small" "json" "lz4"
  compress_loop "small" "json" "gzip"

  echo "Compressed JSON"

  # # logfmt
  compress_loop "small" "logfmt" "lz4"
  compress_loop "small" "logfmt" "gzip"

  echo "Compressed LOGFMT"

  printf '\n]\n}\n' >> "$CREATION"
}

compress_loop() {
  local size="$1" file_type="$2" scheme="$3"
  local in="$BENCH_DATA/all_input_data/${file_type}/${size}_data"
  local out="$BENCH_DATA/tmp_compressed/${file_type}/${size}_data"
  local i label src dst elapsed max_rss base_rss data_size index_size

  for i in {1..4}; do
    label="${scheme},${size},${file_type},$((i-1))"

    echo "Running ${label}"

    src="${in}/data_${i}/${i}_${size}.${file_type}"
    dst="${out}/data_${i}/${i}_${size}.${scheme}"

    if [[ "$scheme" == lz4 ]]; then
      "$GTIME" -o "$TIMES" -f '%M' lz4 -q -f "$BASE_DIR/empty" "$BASE_DIR/out.lz4"
      base_rss=$(tail -n1 "$TIMES")
      "$GTIME" -o "$TIMES" -f '%e %M' lz4 -q -f "$src" "$dst"
    elif [[ "$scheme" == gzip ]]; then
      "$GTIME" -o "$TIMES" -f '%M' gzip -c "$BASE_DIR/empty" >"$BASE_DIR/out.gzip"
      base_rss=$(tail -n1 "$TIMES")
      "$GTIME" -o "$TIMES" -f '%e %M' gzip -c "$src" >"$dst"
    else
      echo "unknown scheme: $scheme" >&2
      return 1
    fi

    read -r elapsed max_rss < <(tail -n1 "$TIMES")

    data_size=$(stat -c%s "$src")
    index_size=$(stat -c%s "$dst")

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "peak_delta": %s,\n    "index_size": %s,\n    "data_size": %s\n  }' \
      "$sep" "$label" "$elapsed" "$(( (max_rss - base_rss) * 1024 ))" "$index_size" "$data_size" >> "$CREATION"
    sep=$',\n'
  done
}

run_bench() {
  local size="$1" file_type="$2" data_set="$3" query="$4" scheme="$5"
  local file_path="$BENCH_DATA/tmp_compressed/${file_type}/${size}_data/data_${data_set}/${data_set}_${size}.${scheme}"
  local label="${scheme},${size},${file_type},$((data_set-1))"
  local i elapsed max_rss base_rss dec_elapsed dec_rss grep_elapsed grep_rss
  for i in $(seq 1 20); do
    if [[ "$scheme" != lz4 && "$scheme" != gzip ]]; then
      echo "unknown scheme: $scheme" >&2
      return 1
    fi

    "$GTIME" -o "$TIMES"  -f '%M' "$scheme" -dc "$BASE_DIR/empty.$scheme" \
      | "$GTIME" -o "$TIMES2" -f '%M' grep -c "$query" >/dev/null || true
    base_rss=$(( $(tail -n1 "$TIMES") + $(tail -n1 "$TIMES2") ))

    "$GTIME" -o "$TIMES"  -f '%e %M' "$scheme" -dc "$file_path" \
      | "$GTIME" -o "$TIMES2" -f '%e %M' grep -c "$query" >/dev/null || true

    read -r dec_elapsed dec_rss < <(tail -n1 "$TIMES")
    read -r grep_elapsed grep_rss < <(tail -n1 "$TIMES2")
    elapsed=$(awk -v a="$dec_elapsed" -v b="$grep_elapsed" 'BEGIN { print (a > b ? a : b) }')
    max_rss=$(( dec_rss + grep_rss ))

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "peak_delta": %s\n  }' \
      "$sep" "$label" "$elapsed" "$(( (max_rss - base_rss) * 1024 ))" >> "$SEARCH"
    sep=$',\n'
  done
}

rm -f "$CREATION" "$SEARCH"
rm -rf "$BENCH_DATA/tmp_compressed"

setup_compress_data

queries=("book" "IssueCommentEvent" "germany" "2026")
printf '{\n"benchmarks": [\n' >> "$SEARCH"
sep=""
for data_set in {1..4}; do
  for size in small; do
    for file_type in csv; do
      for scheme in lz4 gzip; do
        run_bench "${size}" "${file_type}" "${data_set}" "${queries[data_set-1]}" "${scheme}"
      done
    done
  done
done
printf '\n]\n}\n' >> "$SEARCH"

rm -rf "$BENCH_DATA/tmp_compressed"

echo "done: $CREATION $SEARCH"