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

rss_kb() { awk -v k="$2:" '$1==k {print $2}' "/proc/$1/status"; }

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
  local size="$1" scheme="$2" all_fields=("${@:3}")
  local in="$BENCH_DATA/all_input_data/json/${size}_data"
  local out="$BENCH_DATA/tmp_indexed/json/${scheme}/${size}_data"
  local i label src dst start end elapsed_ms max_rss base_rss data_size index_size

  for i in {1..4}; do
    local label="${scheme},${size},$((i-1))"
    local fields="${all_fields[i-1]}"

    echo "Running ${label}"
    echo "$fields"

    src="${in}/data_${i}"
    dst="${out}/data_${i}"

    read -ra field_list <<< "$fields"

    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/indexer" --help >/dev/null
    base_rss=$(tail -n1 "$TIMES")
    start=$("$DATE" +%s%N)
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/indexer" --mode create --encoding "$scheme" -d "$src" -i "$dst" --fields "${field_list[@]}" --threads 1
    end=$("$DATE" +%s%N)

    elapsed_ms=$(( (end - start) / 1000000 ))
    max_rss=$(tail -n1 "$TIMES")

    data_size=$("$DU" -sb "$src" | cut -f1)
    index_size=$("$DU" -sb "$dst" | cut -f1)

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ms",\n    "peak_delta": %s,\n    "index_size": %s,\n    "data_size": %s\n  }' \
      "$sep" "$label" "$elapsed_ms" "$(( (max_rss - base_rss) * 1024 ))" "$index_size" "$data_size" >> "$CREATION_SIMDINDEX"
    sep=$',\n'
  done
}

elastic_create_loop() {
  local size="$1" all_fields=("${@:2}")
  local in="$BENCH_DATA/all_input_data/json/${size}_data"
  local i label src dst start end elapsed_ms max_rss base_rss data_size index_size es_pid resp

  for i in {1..4}; do
    label="elastic,${size},$((i-1))"
    idx_name="${size}_$((i-1))_idx"
    local fields="${all_fields[i-1]}"
    local tmp="$(mktemp -d)"

    echo "Running ${label}"

    src="${in}/data_${i}/${i}_small.json"

    split -a 4 -C 20M "$src" "$tmp/chunk_"

    for chunk in "$tmp"/chunk_*; do
            jq -c '{"index":{}}, .' "$chunk" > "$chunk.json" || { rm -rf "$tmp"; return 1; }
      rm -f "$chunk"
    done

    curl -sS -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" -X DELETE "http://localhost:9200/$idx_name"

    es_pid="$(pgrep -f org.elasticsearch.bootstrap.Elasticsearch | head -n1)"
    if [[ -z "$es_pid" ]]; then
      echo "Elasticsearch process not found" >&2
      rm -rf "$tmp"
      return 1
    fi

    base_rss="$(rss_kb "$es_pid" VmRSS)"
    echo 5 | sudo tee "/proc/$es_pid/clear_refs" >/dev/null

    start=$("$DATE" +%s%N)
    curl -sS --fail-with-body -u "elastic:$ES_LOCAL_PASSWORD" \
        -H 'Content-Type: application/json' \
        -X PUT "http://localhost:9200/$idx_name" -d '{
      "settings": { 
        "number_of_shards": 1, 
        "number_of_replicas": 0,
        "analysis": { "analyzer": { "words": { "type": "pattern", "pattern": "[\\W_]+", "flags": "UNICODE_CHARACTER_CLASS", "lowercase": true } } }
      },
      "mappings": {
        "_source": { "enabled": false },
        "dynamic": false,
        '"$fields"'
      }
    }'

    for chunk in "$tmp"/chunk_*.json; do
      resp="$(curl -sS --fail-with-body -u "elastic:$ES_LOCAL_PASSWORD" \
        -H 'Content-Type: application/x-ndjson' \
        -X POST "http://localhost:9200/$idx_name/_bulk?filter_path=errors,items.*.error" \
        --data-binary @"$chunk")"
      if [[ "$resp" != '{"errors":false}' ]]; then
        echo "bulk errors in $chunk:" >&2
        echo "$resp" | head -c 2000 >&2
        echo >&2
        rm -rf "$tmp"
        return 1
      fi
    done

    curl -sS -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" -X POST "http://localhost:9200/$idx_name/_refresh"
    curl -sS -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" -X POST "http://localhost:9200/$idx_name/_forcemerge?max_num_segments=3"

    end=$("$DATE" +%s%N)

    curl -sS -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" -X POST "http://localhost:9200/$idx_name/_refresh"

    max_rss="$(rss_kb "$es_pid" VmHWM)"
    elapsed_ms=$(( (end - start) / 1000000 ))

    rm -rf "$tmp"

    data_size=$("$STAT" -c%s "$src")
    index_size=$(curl -sS -u "elastic:$ES_LOCAL_PASSWORD" "http://localhost:9200/_cat/indices/$idx_name?h=store.size&bytes=b" | tr -d '[:space:]')
 
    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ms",\n    "peak_delta": %s,\n    "index_size": %s,\n    "data_size": %s\n  }' \
      "$sep" "$label" "$elapsed_ms" "$(( (max_rss - base_rss) * 1024 ))" "$index_size" "$data_size" >> "$CREATION_ELASTIC"
    sep=$',\n'
  done
}

simdindex_search_loop() {
  local size="$1" scheme="$2" querys=("${@:3}")
  local path="$BENCH_DATA/tmp_indexed/${file_type}/${scheme}/${size}_data"
  local i label src dst start end elapsed_ms max_rss base_rss data_size index_size

  for i in {1..4}; do
    local query="${querys[i-1]}"
    local label="${scheme},${size},$((i-1))"
    
    local idx="${path}/data_${i}"


    echo "Running ${label}"

    src="${in}/data_${i}"
    dst="${out}/data_${i}"

    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/indexer" --help >/dev/null
    base_rss=$(tail -n1 "$TIMES")
    start=$("$DATE" +%s%N)
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/indexer" --mode search -i "$idx" --query "$query" --threads 1 
    end=$("$DATE" +%s%N)

    elapsed_ms=$(( (end - start) / 1000000 ))
    max_rss=$(tail -n1 "$TIMES")

    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ms",\n    "peak_delta": %s,\n  }' \
      "$sep" "$label" "$elapsed_ms" "$(( (max_rss - base_rss) * 1024 ))" >> "$SEARCH_SIMDINDEX"
    sep=$',\n'
  done
}

elastic_search_loop() {
  local size="$1" scheme="$2" querys=("${@:3}")
  local path="$BENCH_DATA/tmp_indexed/${file_type}/${size}_data"
  local i src dst start end elapsed_ms max_rss base_rss data_size index_size

  for i in {1..4}; do
    local query="${querys[i-1]}"
    local label="elastic,${size},$((i-1))"
    local idx_name="${size}_$((i-1))_idx"

    curl -sS -u "elastic:$ES_LOCAL_PASSWORD" \
    -H 'Content-Type: application/json' \
    -X GET "http://127.0.0.1:9200/$idx_name/_count" \
    -d "$query" \
    -w '\ndns %{time_namelookup}\nconnect %{time_connect}\nttfb %{time_starttransfer}\ntotal %{time_total}\n'
}

simdindex_fields=(
  "overall reviewText"
  "type id"
  "page_title timestamp"
  "publication_year language"
)

simdindex_simple_queries=(
  '[reviewText] = "book"'
  '[page_title] = "IssueCommentEvent"'
  '[page_title] = "germany"'
  '[publication_year] = "2026"'
)

simdindex_union_queries=(
  '[reviewText] = "book" | [reviewText] = "television"'
  '[type] = "IssueCommentEvent" | [type] = "PullRequestReviewCommentEvent"'
  '[page_title] = "germany" | [page_title] = "england"'
  '[publication_year] = "2026" | [publication_year] = "2025"'
)

simdindex_intersection_queries=(
  '[reviewText] = "book" & [reviewText] = "paperback"'
  '[type] = "IssueCommentEvent" & [id] = "5562772598"'
  '[page_title] = "pdf" & [page_title] = "germany"'
  '[publication_year] = "2026" & [language] = "en"'
)

simdindex_combination_queries=(
  '([overall] = "5.0" & [reviewText] = "book") & [reviewText] = "paperback"'
  '([type] = "IssueCommentEvent" | [type] = "PullRequestReviewCommentEvent") & [id] = "5562772598"'
  '([page_title] = "germany" | [page_title] = "england") | [page_title] = "pdf"'
  '([publication_year] = "2026" | [publication_year] = "2025") & [language] = "en"'
)

simdindex_complex_combination_queries=(
  '([overall] = "5.0" & [reviewText] = "paperback") | [overall] = {3.0,4.0}'
  '([type] = "IssueCommentEvent" | [type] = "PullRequestReviewCommentEvent") & [id] = {1162772598,5862772598}'
  '([page_title] = "germany" | [page_title] = "england") & ([page_title] = "pdf" | [page_title] = ".html")'
  '([publication_year] = {2020,2025} & [language] = "en") | [publication_year] = "2019"'
)

elastic_fields=(
  '"properties": {
    "reviewText": { "type": "text", "page_title": { "type": "text", "analyzer": "words", "index_options": "docs", "norms": false },
    "overall":   { "type": "float" }
  }'
  '"properties": {
    "type": { "type": "text", "page_title": { "type": "text", "analyzer": "words", "index_options": "docs", "norms": false },
    "id":   { "type": "long" }
  }'
  '"properties": {
    "page_title": { "type": "text", "page_title": { "type": "text", "analyzer": "words", "index_options": "docs", "norms": false },
    "timestamp":   { "type": "date", "format": "yyyy-MM-dd HH:mm:ss" }
  }'
  '"properties": {
    "language": { "type": "text", "page_title": { "type": "text", "analyzer": "words", "index_options": "docs", "norms": false },
    "publication_year":   { "type": "integer" }
  }'
)

elastic_simple_queries=(
  '{ "query": { "query_string": { "query": "reviewText:book" } } }'
  '{ "query": { "query_string": { "query": "type:IssueCommentEvent" } } }'
  '{ "query": { "query_string": { "query": "page_title:germany" } } }'
  '{ "query": { "query_string": { "query": "publication_year:2026" } } }'
)

elastic_union_queries=(
  '{ "query": { "query_string": { "query": "reviewText:(book OR paperback)" } } }'
  '{ "query": { "query_string": { "query": "type:(IssueCommentEvent OR PullRequestReviewCommentEvent)" } } }'
  '{ "query": { "query_string": { "query": "page_title:(germany OR england)" } } }'
  '{ "query": { "query_string": { "query": "publication_year:(2026 OR 2025)" } } }'
)

elastic_intersection_queries=(
  '{ "query": { "query_string": { "query": "reviewText:(book AND television)" } } }'
  '{ "query": { "query_string": { "query": "type:IssueCommentEvent AND id:5562772598" } } }'
  '{ "query": { "query_string": { "query": "page_title:(germany AND england)" } } }'
  '{ "query": { "query_string": { "query": "publication_year:2026 AND language:en" } } }'
)

elastic_combination_queries=(
  '{ "query": { "query_string": { "query": "overall:5.0 AND reviewText:(book AND paperback)" } } }'
  '{ "query": { "query_string": { "query": "type:(IssueCommentEvent OR PullRequestReviewCommentEvent) AND id:5562772598" } } }'
  '{ "query": { "query_string": { "query": "page_title:(germany OR england) OR page_title:pdf" } } }'
  '{ "query": { "query_string": { "query": "publication_year:(2026 OR 2025) AND language:en" } } }'
)

elastic_complex_combination_queries=(
  '{ "query": { "query_string": { "query": "(overall:5.0 AND reviewText:paperback) OR overall:[3.0 TO 4.0]" } } }'
  '{ "query": { "query_string": { "query": "type:(IssueCommentEvent OR PullRequestReviewCommentEvent) AND id:[1162772598 TO 5862772598]" } } }'
  '{ "query": { "query_string": { "query": "page_title:(germany OR england) OR page_title:(pdf OR .html)" } } }'
  '{ "query": { "query_string": { "query": "(publication_year:[2020 TO 2025] AND language:en) OR publication_year:2019" } } }'
)

rm -f "$CREATION_SIMDINDEX" "$CREATION_ELASTIC"
rm -f "SEARCH_SIMDINDEX" "SEARCH_ELASTIC"
rm -rf "$BENCH_DATA"/tmp_indexed

mkdir -p "$BENCH_DATA"/tmp_indexed/{csv,json,logfmt}/small_data/data_{1..4}

schemes="gvb pef fls s4_bpd4 s4_bpdm s8_bpd4 s8_bpdm s16_bpd4 s16_bpdm"

printf '{\n"benchmarks": [\n' > "$CREATION_SIMDINDEX"
sep=""
for scheme in "${schemes[@]}"; do
  simdindex_create_loop "small" "$scheme" "${simdindex_fields[@]}"
done
printf '\n]\n}\n' >> "$CREATION_SIMDINDEX"

printf '{\n"benchmarks": [\n' > "$CREATION_ELASTIC"
sep=""
elastic_create_loop "small" "${elastic_fields[@]}"
printf '\n]\n}\n' >> "$CREATION_ELASTIC"

sep=""
for scheme in "${schemes[@]}"; do
  for query_type in simdindex_{simple,union,intersection,combination,complex_combination}_queries; do
    ref="${query_type}[@]"
    simdindex_search_loop "small" "$scheme" "${!ref}"
  done
done

sep=""
for query_type in elastic_{simple,union,intersection,combination,complex_combination}_queries; do
  ref="${query_type}[@]"
  elastic_search_loop "small" "${!ref}"
done