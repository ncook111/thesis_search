#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/../misc/paths.sh"
CREATION_OUT_DIR="$RESULTS_DIR/creation"
SEARCH_OUT_DIR="$RESULTS_DIR/creation"

mkdir -p "$CREATION_OUT_DIR"
mkdir -p "$SEARCH_OUT_DIR"

GTIME=/usr/bin/time

CREATION="$CREATION_OUT_DIR/creation_gz_lz4.json"
TIMES=$(mktemp)
trap 'rm -f "$TIMES"' EXIT
SEARCH="$SEARCH_OUT_DIR/search_gz_lz4.json"
sep=""

######## Compress Data ########
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
  local i label src dst output elapsed compression

  for i in {1..4}; do
    label="${scheme},${size},${file_type},$((i-1))"

    echo "Running ${label}"

    src="${in}/data_${i}/${i}_${size}.${file_type}"
    dst="${out}/data_${i}/${i}_${size}.${scheme}"

    if [[ "$scheme" == lz4 ]]; then
      output=$("$GTIME" -f 'ELAPSED %e' lz4 -q -f "$src" "$dst" 2>&1)
    elif [[ "$scheme" == gzip ]]; then
      output=$("$GTIME" -f 'ELAPSED %e' gzip -c "$src" 2>&1 >"$dst")
    else
      echo "unknown scheme: $scheme" >&2
      return 1
    fi

    elapsed=$(grep -o 'ELAPSED [0-9.]*' <<<"$output" | awk '{print $2}')
    compression=$(awk -v o="$(stat -c%s "$src")" -v c="$(stat -c%s "$dst")" \
                      'BEGIN { printf "%.2f", c / o * 100 }')

    printf '%s  {\n    "label": "%s",\n    "compression": %s,\n    "real_time": %s\n  }' \
      "$sep" "$label" "${compression}" "${elapsed}" >> "$CREATION"
    sep=$',\n'
  done
}

run_bench() {
  local size="$1" file_type="$2" data_set="$3" query="$4" scheme="$5"
  local file_path="$BENCH_DATA/tmp_compressed/${file_type}/${size}_data/data_${data_set}/${data_set}_${size}.${scheme}"
  local label="${scheme},${size},${file_type},$((data_set-1))"
  local i elapsed
  for i in $(seq 1 20); do
    if [[ "$scheme" == lz4 ]]; then
      "$GTIME" -o "$TIMES" -f "%e" bash -c "lz4 -dc '${file_path}' | grep -c '${query}' &>/dev/null || true"
    elif [[ "$scheme" == gzip ]]; then
      "$GTIME" -o "$TIMES" -f "%e" bash -c "gzip -dc '${file_path}' | grep -c '${query}' &>/dev/null || true"
    else
      echo "unknown scheme: $scheme" >&2
      return 1
    fi
    elapsed=$(<"$TIMES")

    printf '%s  {\n    "label": "%s",\n    "real_time": %s\n  }' \
      "$sep" "$label" "${elapsed}" >> "$SEARCH"
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