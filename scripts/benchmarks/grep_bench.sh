#!/usr/bin/env bash
set -Eeuo pipefail
# Strict mode exits on the first failing command; say which one instead of exiting silently.
trap 'rc=$?; echo "error: line $LINENO exited with status $rc: $BASH_COMMAND" >&2' ERR

source "$(dirname "$0")/../misc/paths.sh"
CREATION_OUT_DIR="$RESULTS_DIR/creation_grep"
SEARCH_OUT_DIR="$RESULTS_DIR/search_grep"

mkdir -p "$CREATION_OUT_DIR"
mkdir -p "$SEARCH_OUT_DIR"

if [[ "$(uname)" == Darwin ]]; then
  export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
  GTIME=gtime DATE=gdate STAT=gstat DU=gdu
else
  GTIME=/usr/bin/time DATE=date STAT=stat DU=du
fi

CREATION_GREP="$CREATION_OUT_DIR/creation_grep.json"
CREATION_SIMDINDEX="$CREATION_OUT_DIR/creation_simdindex.json"
TIMES=$(mktemp)
TIMES2=$(mktemp)
BASE_DIR=$(mktemp -d)
trap 'rm -f "$TIMES" "$TIMES2"; rm -rf "$BASE_DIR"' EXIT

SEARCH_GREP_WARM="$SEARCH_OUT_DIR/search_grep_warm.json"
SEARCH_GREP_COLD="$SEARCH_OUT_DIR/search_grep_cold.json"
SEARCH_SIMDINDEX_WARM="$SEARCH_OUT_DIR/search_simdindex_warm.json"
SEARCH_SIMDINDEX_COLD="$SEARCH_OUT_DIR/search_simdindex_cold.json"
sep=""

: > "$BASE_DIR/empty"
lz4 -q -f "$BASE_DIR/empty" "$BASE_DIR/empty.lz4"
gzip -c "$BASE_DIR/empty" > "$BASE_DIR/empty.gzip"

# Flush the Linux page cache so the next read comes from disk. Needs root.
drop_caches() {
  sync
  if [[ $EUID -eq 0 ]]; then
    echo 3 > /proc/sys/vm/drop_caches
  else
    echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null
  fi
}

grep_create_loop() {
  mkdir -p "$BENCH_DATA"/tmp_compressed/{csv,json,logfmt}/small_data/data_{1..4}

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
}

compress_loop() {
  local size="$1" file_type="$2" scheme="$3"
  local in="$BENCH_DATA/all_input_data/${file_type}/${size}_data"
  local out="$BENCH_DATA/tmp_compressed/${file_type}/${size}_data"
  local i label src dst start end elapsed max_rss base_rss data_size index_size

  for i in {1..4}; do
    label="${scheme},${size},${file_type},$((i-1))"

    echo "Running ${label}"

    src="${in}/data_${i}/${i}_${size}.${file_type}"
    dst="${out}/data_${i}/${i}_${size}.${scheme}"

    if [[ "$scheme" == lz4 ]]; then
      "$GTIME" -o "$TIMES" -f '%M' lz4 -q -f "$BASE_DIR/empty" "$BASE_DIR/out.lz4"
      base_rss=$(tail -n1 "$TIMES")
      start=$("$DATE" +%s%N)
      "$GTIME" -o "$TIMES" -f '%M' lz4 -q -f "$src" "$dst"
      end=$("$DATE" +%s%N)
    elif [[ "$scheme" == gzip ]]; then
      "$GTIME" -o "$TIMES" -f '%M' gzip -c "$BASE_DIR/empty" >"$BASE_DIR/out.gzip"
      base_rss=$(tail -n1 "$TIMES")
      start=$("$DATE" +%s%N)
      "$GTIME" -o "$TIMES" -f '%M' gzip -c "$src" >"$dst"
      end=$("$DATE" +%s%N)
    else
      echo "unknown scheme: $scheme" >&2
      return 1
    fi

    elapsed=$(( (end - start) ))
    max_rss=$(tail -n1 "$TIMES")

    data_size=$("$STAT" -c%s "$src")
    index_size=$("$STAT" -c%s "$dst")

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ns",\n    "peak_delta": %s,\n    "index_size": %s,\n    "data_size": %s\n  }' \
      "$sep" "$label" "$elapsed" "$(( (max_rss - base_rss) * 1024 ))" "$index_size" "$data_size" >> "$CREATION_GREP"
    sep=$',\n'
  done
}

simdindex_create_loop() {
  local size="$1" scheme="$2" file_type="$3"
  local in="$BENCH_DATA/all_input_data/${file_type}/${size}_data"
  local out="$BENCH_DATA/tmp_indexed/${file_type}/${scheme}/${size}_data"
  local i label src dst start end elapsed max_rss base_rss data_size index_size

  for i in {1..4}; do
    local label="${scheme},${size},${file_type},$((i-1))"

    echo "Running ${label}"

    src="${in}/data_${i}"
    dst="${out}/data_${i}"

    # Baseline only needs the memory figure; the exit status of --help does not matter.
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/simdindex" --help >/dev/null || true
    base_rss=$(tail -n1 "$TIMES")
    start=$("$DATE" +%s%N)
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/simdindex" --mode create --encoding "$scheme" -d "$src" -i "$dst" --threads 1
    end=$("$DATE" +%s%N)

    elapsed=$(( (end - start) ))
    max_rss=$(tail -n1 "$TIMES")

    data_size=$("$DU" -sb "$src" | cut -f1)
    index_size=$("$DU" -sb "$dst" | cut -f1)

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ns",\n    "peak_delta": %s,\n    "index_size": %s,\n    "data_size": %s\n  }' \
      "$sep" "$label" "$elapsed" "$(( (max_rss - base_rss) * 1024 ))" "$index_size" "$data_size" >> "$CREATION_SIMDINDEX"
    sep=$',\n'
  done
}

simdindex_search_loop() {
  local size="$1" file_type="$2" data_set="$3" query="$4" scheme="$5" cold="${6:-false}"
  # Same location simdindex_create_loop writes to.
  local idx="$BENCH_DATA/tmp_indexed/${file_type}/${scheme}/${size}_data/data_${data_set}"
  local label="${scheme},${size},${file_type},$((data_set-1))"
  local i start end elapsed max_rss base_rss

  echo "Running ${label}"

  # Warm mode: one untimed run so every timed repetition reads from the page cache.
  if [[ "$cold" != true ]]; then
    "$BUILD_DIR/simdindex" --mode search -i "$idx" --query "$query" --threads 1 >/dev/null
  fi

  # 20 repetitions of one query against one index, matching grep_search_loop.
  for i in $(seq 1 20); do
    # Cold mode: flush before every repetition. The baseline run that follows
    # reloads the simdindex binary, so only the index itself is read cold.
    if [[ "$cold" == true ]]; then
      drop_caches
    fi

    # Baseline only needs the memory figure; the exit status of --help does not matter.
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/simdindex" --help >/dev/null || true
    base_rss=$(tail -n1 "$TIMES")
    start=$("$DATE" +%s%N)
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/simdindex" --mode search -i "$idx" --query "$query" --threads 1 >/dev/null
    end=$("$DATE" +%s%N)

    elapsed=$(( (end - start) ))
    max_rss=$(tail -n1 "$TIMES")

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ns",\n    "peak_delta": %s\n  }' \
      "$sep" "$label" "$elapsed" "$(( (max_rss - base_rss) * 1024 ))" >> "$SEARCH_SIMDINDEX"
    sep=$',\n'
  done
}

grep_search_loop() {
  local size="$1" file_type="$2" data_set="$3" query="$4" scheme="$5" cold="${6:-false}"
  local file_path="$BENCH_DATA/tmp_compressed/${file_type}/${size}_data/data_${data_set}/${data_set}_${size}.${scheme}"
  local label="${scheme},${size},${file_type},$((data_set-1))"
  local i start end elapsed max_rss base_rss dec_rss grep_rss

  if [[ "$scheme" != lz4 && "$scheme" != gzip ]]; then
    echo "unknown scheme: $scheme" >&2
    return 1
  fi

  # Warm mode: one untimed run so every timed repetition reads from the page cache.
  if [[ "$cold" != true ]]; then
    "$scheme" -dc "$file_path" | grep -wci "$query" >/dev/null || true
  fi

  for i in $(seq 1 20); do
    # Cold mode: flush before every repetition. The baseline run that follows
    # reloads the decompressor and grep binaries, so only the data file is read cold.
    if [[ "$cold" == true ]]; then
      drop_caches
    fi

    "$GTIME" -o "$TIMES"  -f '%M' "$scheme" -dc "$BASE_DIR/empty.$scheme" \
      | "$GTIME" -o "$TIMES2" -f '%M' grep -wci "$query" >/dev/null || true
    base_rss=$(( $(tail -n1 "$TIMES") + $(tail -n1 "$TIMES2") ))

    start=$("$DATE" +%s%N)
    "$GTIME" -o "$TIMES"  -f '%M' "$scheme" -dc "$file_path" \
      | "$GTIME" -o "$TIMES2" -f '%M' grep -wci "$query" >/dev/null || true
    end=$("$DATE" +%s%N)

    elapsed=$(( (end - start) ))
    dec_rss=$(tail -n1 "$TIMES")
    grep_rss=$(tail -n1 "$TIMES2")
    max_rss=$(( dec_rss + grep_rss ))

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ns",\n    "peak_delta": %s\n  }' \
      "$sep" "$label" "$elapsed" "$(( (max_rss - base_rss) * 1024 ))" >> "$SEARCH_GREP"
    sep=$',\n'
  done
}

rm -f "$CREATION_SIMDINDEX" "$CREATION_GREP"
rm -f "$SEARCH_SIMDINDEX_WARM" "$SEARCH_SIMDINDEX_COLD" "$SEARCH_GREP_WARM" "$SEARCH_GREP_COLD"
rm -rf "$BENCH_DATA"/tmp_indexed
rm -rf "$BENCH_DATA/tmp_compressed"

schemes=(gvb pef fls s4_bpd4 s4_bpdm s8_bpd4 s8_bpdm s16_bpd4 s16_bpdm)

for scheme in "${schemes[@]}"; do
  mkdir -p "$BENCH_DATA"/tmp_indexed/{csv,json,logfmt}/"$scheme"/small_data/data_{1..4}
done

printf '{\n"benchmarks": [\n' > "$CREATION_SIMDINDEX"
sep=""
for scheme in "${schemes[@]}"; do
  for file_type in csv json logfmt; do
    simdindex_create_loop "small" "$scheme" "$file_type"
  done
done
printf '\n]\n}\n' >> "$CREATION_SIMDINDEX"

printf '{\n"benchmarks": [\n' >> "$CREATION_GREP"
sep=""
grep_create_loop
printf '\n]\n}\n' >> "$CREATION_GREP"

simdindex_queries=(
  '[*] = "book"'
  '[*] = "IssueCommentEvent"'
  '[*] = "germany"'
  '[*] = "2026"'
)

for cold in false true; do
  if [[ "$cold" == true ]]; then
    SEARCH_SIMDINDEX="$SEARCH_SIMDINDEX_COLD"
  else
    SEARCH_SIMDINDEX="$SEARCH_SIMDINDEX_WARM"
  fi

  printf '{\n"benchmarks": [\n' >> "$SEARCH_SIMDINDEX"
  sep=""
  for data_set in {1..4}; do
    for size in small; do
      for file_type in csv json logfmt; do
        for scheme in "${schemes[@]}"; do
          simdindex_search_loop "$size" "$file_type" "$data_set" "${simdindex_queries[data_set-1]}" "$scheme" "$cold"
        done
      done
    done
  done
  printf '\n]\n}\n' >> "$SEARCH_SIMDINDEX"
done


grep_queries=("book" "IssueCommentEvent" "germany" "2026")
for cold in false true; do
  if [[ "$cold" == true ]]; then
    SEARCH_GREP="$SEARCH_GREP_COLD"
  else
    SEARCH_GREP="$SEARCH_GREP_WARM"
  fi

  printf '{\n"benchmarks": [\n' >> "$SEARCH_GREP"
  sep=""
  for data_set in {1..4}; do
    for size in small; do
      for file_type in csv json logfmt; do
        for scheme in lz4 gzip; do
          grep_search_loop "$size" "$file_type" "$data_set" "${grep_queries[data_set-1]}" "$scheme" "$cold"
        done
      done
    done
  done
  printf '\n]\n}\n' >> "$SEARCH_GREP"
done

rm -rf "$BENCH_DATA/tmp_compressed"