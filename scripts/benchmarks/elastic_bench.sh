#!/usr/bin/env bash
#set -euo pipefail

source "$(dirname "$0")/../misc/paths.sh"
source ~/elastic-start-local/.env

CREATION_OUT_DIR="$RESULTS_DIR/creation_elastic"
SEARCH_OUT_DIR="$RESULTS_DIR/search_elastic"

mkdir -p "$CREATION_OUT_DIR"
mkdir -p "$SEARCH_OUT_DIR"

if [[ "$(uname)" == Darwin ]]; then
  export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
  GTIME=gtime DATE=gdate STAT=gstat DU=gdu
else
  GTIME=/usr/bin/time DATE=date STAT=stat DU=du
fi

CREATION_ELASTIC="$CREATION_OUT_DIR/creation_elastic.json"
CREATION_SIMDINDEX="$CREATION_OUT_DIR/creation_simdindex.json"
TIMES=$(mktemp)
TIMES2=$(mktemp)
BASE_DIR=$(mktemp -d)
trap 'rm -f "$TIMES" "$TIMES2"; rm -rf "$BASE_DIR"' EXIT

SEARCH_ELASTIC="$SEARCH_OUT_DIR/search_elastic.json"
SEARCH_SIMDINDEX="$SEARCH_OUT_DIR/search_simdindex.json"
sep=""

FILE="$BENCH_DATA/all_input_data/json/small_data/data_1/1_small.json"
INDEX="my_index"

# tmp="$(mktemp -d)"
# trap 'rm -rf "$tmp"' EXIT

# split -a 4 -l 10000 "$FILE" "$tmp/chunk_"

# curl -sS -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" -X DELETE "http://localhost:9200/$INDEX"

# curl -sS --fail-with-body -u "elastic:$ES_LOCAL_PASSWORD" \
#     -H 'Content-Type: application/json' \
#     -X PUT "http://localhost:9200/$INDEX" -d '{
#   "settings": { "number_of_shards": 1, "number_of_replicas": 0 },
#   "mappings": {
#     "_source": { "enabled": false },
#     "dynamic": false,
#     "properties": {
#       "page_title": { "type": "text", "index_options": "docs", "norms": false },
#       "count_views":   { "type": "integer" }
#     }
#   }
# }'

# export ES_LOCAL_PASSWORD INDEX

# bulk_one() {
#     : "${ES_LOCAL_PASSWORD:?not exported}" "${INDEX:?not exported}"
#     local resp
#     resp="$(awk 'NF { print "{\"index\":{}}"; print }' "$1" |
#         curl -sS --fail-with-body -u "elastic:$ES_LOCAL_PASSWORD" \
#             -H 'Content-Type: application/x-ndjson' \
#             -X POST "http://127.0.0.1:9200/$INDEX/_bulk?filter_path=errors,items.*.error" \
#             --data-binary @-)" || { echo "curl failed on $1: ${resp:0:500}" >&2; return 255; }
#     [[ "$resp" == '{"errors":false}' ]] || { echo "bulk errors in $1: ${resp:0:2000}" >&2; return 255; }
# }
# export -f bulk_one

# find "$tmp" -name 'chunk_*' -print0 |
#     xargs -0 -n 1 -P 10 bash -c 'bulk_one "$1"' _

# curl -sS -u "elastic:$ES_LOCAL_PASSWORD" \
#   -H 'Content-Type: application/json' \
#   -X GET "http://127.0.0.1:9200/my_index/_count" \
#   -d '{ "query": { "match": { "count_views": "1" } } }' \
#   -w '\ndns %{time_namelookup}\nconnect %{time_connect}\nttfb %{time_starttransfer}\ntotal %{time_total}\n'

# curl -sS -u "elastic:$ES_LOCAL_PASSWORD" \
#   -H 'Content-Type: application/json' \
#   -X GET "http://localhost:9200/my_index/_count" \
#   -d '{ "query": { "match": { "type": "IssueCommentEvent" } } }'

# curl -sS -u "elastic:$ES_LOCAL_PASSWORD" \
#   "http://localhost:9200/_cat/indices/$INDEX?v&h=index,docs.count,store.size,pri.store.size"


simdindex_create_loop() {
  local size="$1" file_type="$2" scheme="$3" fields="$4"
  local in="$BENCH_DATA/all_input_data/${file_type}/${size}_data"
  local out="$BENCH_DATA/tmp_indexed/${file_type}/${size}_data"
  local i label src dst start end elapsed_ms max_rss base_rss data_size index_size

  for i in {1..4}; do
    label="${scheme},${size},${file_type},$((i-1))"

    echo "$in"

    echo "Running ${label}"

    src="${in}/data_${i}"
    dst="${out}/data_${i}"

    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/indexer" --help >/dev/null
    base_rss=$(tail -n1 "$TIMES")
    start=$("$DATE" +%s%N)
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/indexer" --mode create --encoding "$scheme" -d "$src" -i "$dst" --fields "$fields" --threads 1
    end=$("$DATE" +%s%N)

    elapsed_ms=$(( (end - start) / 1000000 ))
    max_rss=$(tail -n1 "$TIMES")

    data_size=$("$STAT" -c%s "$src")
    index_size=$("$DU" -sb "$dst" | cut -f1)

    echo "here"

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ms",\n    "peak_delta": %s,\n    "index_size": %s,\n    "data_size": %s\n  }' \
      "$sep" "$label" "$elapsed_ms" "$(( (max_rss - base_rss) * 1024 ))" "$index_size" "$data_size" >> "$CREATION_SIMDINDEX"
    sep=$',\n'
  done
}

mkdir -p "$BENCH_DATA"/tmp_indexed/{csv,json,logfmt}/small_data/data_{1..4}
simdindex_create_loop "small" "json" "s4_bpd4" "page_title timestamp"

# elastic_create_loop() {}

# simdindex_search_loop() {}

# elastic_search_loop() {}