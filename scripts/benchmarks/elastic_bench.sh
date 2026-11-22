#!/usr/bin/env bash
set -Eeuo pipefail
# Strict mode exits on the first failing command; say which one instead of exiting silently.
trap 'rc=$?; echo "error: line $LINENO exited with status $rc: $BASH_COMMAND" >&2' ERR

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
trap 'rm -f "$TIMES"' EXIT

SEARCH_ELASTIC_WARM="$SEARCH_OUT_DIR/search_elastic_warm.json"
SEARCH_ELASTIC_COLD="$SEARCH_OUT_DIR/search_elastic_cold.json"
SEARCH_SIMDINDEX_WARM="$SEARCH_OUT_DIR/search_simdindex_warm.json"
SEARCH_SIMDINDEX_COLD="$SEARCH_OUT_DIR/search_simdindex_cold.json"

rss_kb() { awk -v k="$2:" '$1==k {print $2}' "/proc/$1/status"; }

# Flush the Linux page cache so the next read comes from disk. Needs root.
drop_caches() {
  sync
  if [[ $EUID -eq 0 ]]; then
    echo 3 > /proc/sys/vm/drop_caches
  else
    echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null
  fi
}

simdindex_create_loop() {
  local size="$1" scheme="$2" all_fields=("${@:3}")
  local in="$BENCH_DATA/all_input_data/json/${size}_data"
  local out="$BENCH_DATA/tmp_indexed/json/${scheme}/${size}_data"
  local i label src dst start end elapsed max_rss base_rss data_size index_size

  for i in {1..4}; do
    local label="${scheme},${size},$((i-1))"
    local fields="${all_fields[i-1]}"

    echo "Running ${label}"
    echo "$fields"

    src="${in}/data_${i}"
    dst="${out}/data_${i}"

    read -ra field_list <<< "$fields"

    # Baseline only needs the memory figure; the exit status of --help does not matter.
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/simdindex" --help >/dev/null || true
    base_rss=$(tail -n1 "$TIMES")
    start=$("$DATE" +%s%N)
    "$GTIME" -o "$TIMES" -f '%M' "$BUILD_DIR/simdindex" --mode create --encoding "$scheme" -d "$src" -i "$dst" --fields "${field_list[@]}" --threads 1
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

elastic_create_loop() {
  local size="$1" all_fields=("${@:2}")
  local in="$BENCH_DATA/all_input_data/json/${size}_data"
  local i label src dst start end elapsed max_rss base_rss data_size index_size es_pid resp

  for i in {1..4}; do
    label="elastic,${size},$((i-1))"
    idx_name="${size}_$((i-1))_idx"
    local fields="${all_fields[i-1]}"
    local tmp="$(mktemp -d)"

    echo "Running ${label}"

    src="${in}/data_${i}"

    cat "$src"/* | split -a 4 -C 20M - "$tmp/chunk_"

    for chunk in "$tmp"/chunk_*; do
            jq -c '{"index":{}}, .' "$chunk" > "$chunk.json" || { rm -rf "$tmp"; return 1; }
      rm -f "$chunk"
    done

    curl -sS -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" -X DELETE "http://localhost:9200/$idx_name"

    es_pid="$(pgrep -f org.elasticsearch.bootstrap.Elasticsearch | head -n1 || true)"
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
    elapsed=$(( (end - start) ))

    rm -rf "$tmp"

    data_size=$("$DU" -sb "$src" | cut -f1)
    index_size=$(curl -sS -u "elastic:$ES_LOCAL_PASSWORD" "http://localhost:9200/_cat/indices/$idx_name?h=store.size&bytes=b" | tr -d '[:space:]')
 
    printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ns",\n    "peak_delta": %s,\n    "index_size": %s,\n    "data_size": %s\n  }' \
      "$sep" "$label" "$elapsed" "$(( (max_rss - base_rss) * 1024 ))" "$index_size" "$data_size" >> "$CREATION_ELASTIC"
    sep=$',\n'
  done
}

simdindex_search_loop() {
  # Called as: simdindex_search_loop <size> <scheme> <query type> <cold: true|false> <query for data set 1> ... <query for data set 4>
  local size="$1" scheme="$2" query_type="$3" cold="$4" querys=("${@:5}")
  local path="$BENCH_DATA/tmp_indexed/json/${scheme}/${size}_data"
  local i rep start end elapsed max_rss base_rss

  for i in {1..4}; do
    local query="${querys[i-1]}"
    local label="${scheme},${size},${query_type},$((i-1))"
    local idx="${path}/data_${i}"

    echo "Running ${label}"

    # Warm mode: one untimed run so every timed repetition reads from the page cache.
    if [[ "$cold" != true ]]; then
      "$BUILD_DIR/simdindex" --mode search -i "$idx" --query "$query" --threads 1 >/dev/null
    fi

    for rep in $(seq 1 20); do
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
  done
}

elastic_search_loop() {
  # Called as: elastic_search_loop <size> <query type> <cold: true|false> <query for data set 1> ... <query for data set 4>
  local size="$1" query_type="$2" cold="$3" querys=("${@:4}")
  local i rep elapsed max_rss base_rss es_pid resp rc t

  es_pid="$(pgrep -f org.elasticsearch.bootstrap.Elasticsearch | head -n1 || true)"
  if [[ -z "$es_pid" ]]; then
    echo "Elasticsearch process not found" >&2
    return 1
  fi

  for i in {1..4}; do
    local query="${querys[i-1]}"
    local label="elastic,${size},${query_type},$((i-1))"
    local idx_name="${size}_$((i-1))_idx"

    echo "Running ${label}"

    # Warm mode: one untimed run so every timed repetition reads from the page cache.
    # A failing query is reported by the first timed repetition below.
    if [[ "$cold" != true ]]; then
      curl -sS -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" \
        -H 'Content-Type: application/json' \
        -X GET "http://127.0.0.1:9200/$idx_name/_count" \
        -d "$query" || true
    fi

    for rep in $(seq 1 20); do
      # Cold mode, before every repetition (all untimed): close the index so
      # Elasticsearch releases its mappings of the index files (drop_caches cannot
      # evict pages a process still has mapped), flush the OS page cache, reopen
      # the index, and wait for its shard to be ready.
      if [[ "$cold" == true ]]; then
        curl -sS --fail -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" \
          -X POST "http://127.0.0.1:9200/$idx_name/_close"
        drop_caches
        curl -sS --fail -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" \
          -X POST "http://127.0.0.1:9200/$idx_name/_open"
        curl -sS --fail -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" \
          "http://127.0.0.1:9200/_cluster/health/$idx_name?wait_for_status=green&timeout=60s"
      fi

      # Elasticsearch keeps its own query and request caches, which can answer a
      # repeated identical query without searching. Clear them (untimed) so every
      # repetition is a real search.
      curl -sS -o /dev/null -u "elastic:$ES_LOCAL_PASSWORD" \
        -X POST "http://127.0.0.1:9200/$idx_name/_cache/clear?query=true&request=true"

      # Memory, same method as elastic_create_loop: note the server's current RSS,
      # reset its peak-RSS counter, then read the peak after the query.
      base_rss="$(rss_kb "$es_pid" VmRSS)"
      echo 5 | sudo tee "/proc/$es_pid/clear_refs" >/dev/null

      resp="$(curl -sS --fail-with-body -u "elastic:$ES_LOCAL_PASSWORD" \
        -H 'Content-Type: application/json' \
        -X GET "http://127.0.0.1:9200/$idx_name/_count" \
        -d "$query" \
        -w '\n%{time_total}')" && rc=0 || rc=$?

      t="${resp##*$'\n'}"
      resp="${resp%$'\n'*}"

      if [[ "$rc" -ne 0 ]]; then
        echo "search failed for ${label} (curl exit $rc): $query" >&2
        echo "$resp" | head -c 2000 >&2
        echo >&2
        return 1
      fi

      elapsed=$(( 10#${t/./} * 1000 ))
      max_rss="$(rss_kb "$es_pid" VmHWM)"

      printf '%s  {\n    "label": "%s",\n    "real_time": %s,\n    "time_unit": "ns",\n    "peak_delta": %s\n  }' \
        "$sep" "$label" "$elapsed" "$(( (max_rss - base_rss) * 1024 ))" >> "$SEARCH_ELASTIC"
      sep=$',\n'
    done
  done
}

simdindex_fields=(
  "overall reviewText"
  "type id"
  "page_title timestamp"
  "publication_year language"
)

simdindex_simple_queries=(
  '[reviewText] = "book"'
  '[type] = "IssueCommentEvent"'
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
  '([page_title] = "germany" | [page_title] = "england") & ([page_title] = "pdf" | [page_title] = "html")'
  '([publication_year] = {2020,2025} & [language] = "en") | [publication_year] = "2019"'
)

elastic_fields=(
  '"properties": {
    "reviewText": { "type": "text", "analyzer": "words", "index_options": "docs", "norms": false },
    "overall":   { "type": "float" }
  }'
  '"properties": {
    "type": { "type": "text", "analyzer": "words", "index_options": "docs", "norms": false },
    "id":   { "type": "long" }
  }'
  '"properties": {
    "page_title": { "type": "text", "analyzer": "words", "index_options": "docs", "norms": false },
    "timestamp":   { "type": "date", "format": "yyyy-MM-dd HH:mm:ss" }
  }'
  '"properties": {
    "language": { "type": "text", "analyzer": "words", "index_options": "docs", "norms": false },
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
  '{ "query": { "query_string": { "query": "reviewText:(book OR television)" } } }'
  '{ "query": { "query_string": { "query": "type:(IssueCommentEvent OR PullRequestReviewCommentEvent)" } } }'
  '{ "query": { "query_string": { "query": "page_title:(germany OR england)" } } }'
  '{ "query": { "query_string": { "query": "publication_year:(2026 OR 2025)" } } }'
)

elastic_intersection_queries=(
  '{ "query": { "query_string": { "query": "reviewText:(book AND paperback)" } } }'
  '{ "query": { "query_string": { "query": "type:IssueCommentEvent AND id:5562772598" } } }'
  '{ "query": { "query_string": { "query": "page_title:(pdf AND germany)" } } }'
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
  '{ "query": { "query_string": { "query": "page_title:(germany OR england) AND page_title:(pdf OR html)" } } }'
  '{ "query": { "query_string": { "query": "(publication_year:[2020 TO 2025] AND language:en) OR publication_year:2019" } } }'
)

rm -f "$CREATION_SIMDINDEX" "$CREATION_ELASTIC"
rm -f "$SEARCH_SIMDINDEX_WARM" "$SEARCH_SIMDINDEX_COLD" "$SEARCH_ELASTIC_WARM" "$SEARCH_ELASTIC_COLD"
# Results from before the warm/cold split, so stale files are not mistaken for current ones.
rm -f "$SEARCH_OUT_DIR/search_simdindex.json" "$SEARCH_OUT_DIR/search_elastic.json"
rm -rf "$BENCH_DATA"/tmp_indexed

# schemes=(gvb pef fls s4_bpd4 s4_bpdm s8_bpd4 s8_bpdm s16_bpd4 s16_bpdm)

sizes=(small medium large)

# for size in "${sizes[@]}"; do
#   for scheme in "${schemes[@]}"; do
#     mkdir -p "$BENCH_DATA"/tmp_indexed/json/"$scheme"/"${size}_data"/data_{1..4}
#   done
# done

# printf '{\n"benchmarks": [\n' > "$CREATION_SIMDINDEX"
# sep=""
# for size in "${sizes[@]}"; do
#   for scheme in "${schemes[@]}"; do
#     simdindex_create_loop "$size" "$scheme" "${simdindex_fields[@]}"
#   done
# done
# printf '\n]\n}\n' >> "$CREATION_SIMDINDEX"

query_types=(simple union intersection combination complex_combination)

# for cold in false true; do
#   if [[ "$cold" == true ]]; then
#     SEARCH_SIMDINDEX="$SEARCH_SIMDINDEX_COLD"
#   else
#     SEARCH_SIMDINDEX="$SEARCH_SIMDINDEX_WARM"
#   fi

#   printf '{\n"benchmarks": [\n' > "$SEARCH_SIMDINDEX"
#   sep=""
#   for size in "${sizes[@]}"; do
#     for scheme in "${schemes[@]}"; do
#       for query_type in "${query_types[@]}"; do
#         ref="simdindex_${query_type}_queries[@]"
#         simdindex_search_loop "$size" "$scheme" "$query_type" "$cold" "${!ref}"
#       done
#     done
#   done
#   printf '\n]\n}\n' >> "$SEARCH_SIMDINDEX"
# done

rm -rf "$BENCH_DATA"/tmp_indexed

# printf '{\n"benchmarks": [\n' > "$CREATION_ELASTIC"
# sep=""
# for size in "${sizes[@]}"; do
#   elastic_create_loop "$size" "${elastic_fields[@]}"
# done
# printf '\n]\n}\n' >> "$CREATION_ELASTIC"

for cold in false true; do
  if [[ "$cold" == true ]]; then
    SEARCH_ELASTIC="$SEARCH_ELASTIC_COLD"
  else
    SEARCH_ELASTIC="$SEARCH_ELASTIC_WARM"
  fi

  printf '{\n"benchmarks": [\n' > "$SEARCH_ELASTIC"
  sep=""
  for size in "${sizes[@]}"; do
    for query_type in "${query_types[@]}"; do
      ref="elastic_${query_type}_queries[@]"
      elastic_search_loop "$size" "$query_type" "$cold" "${!ref}"
    done
  done
  printf '\n]\n}\n' >> "$SEARCH_ELASTIC"
done