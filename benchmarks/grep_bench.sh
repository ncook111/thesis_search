#!/bin/bash

######## Compress Data ########
setup_compress_data() {
  mkdir -p bench_data/tmp_compressed/{csv,json,logfmt}/{large,medium,small}_data/data_{1..4}

  # csv
  compress_loop "large" "csv"
  compress_loop "medium" "csv"
  compress_loop "small" "csv"

  # json
  compress_loop "large" "json"
  compress_loop "medium" "json"
  compress_loop "small" "json"

  # # logfmt
  # compress_loop "large" "logfmt"
  # compress_loop "medium" "logfmt"
  # compress_loop "small" "logfmt"
}

compress_loop() {
  local size="$1"
  local file_type="$2"
  local in="bench_data/all_input_data/${file_type}/${size}_data"
  local out="bench_data/tmp_compressed/${file_type}/${size}_data"
  
  for i in {1..3}; do
    ratio=$(lz4 -f "${in}/data_${i}/${i}_${size}.${file_type}" \
                   "${out}/data_${i}/${i}_${size}.lz4" 2>&1 | grep -o '[0-9.]*%' | tail -1)
    echo "${file_type} ${size} data_${i}: ${ratio}" >> ratios.txt
  done

  local ratios=()
  for i in {1..4}; do
    ratio=$(lz4 -f "${in}/data_4/4_${size}_${i}.${file_type}" \
                  "${out}/data_4/4_${size}_${i}.lz4" 2>&1 | grep -o '[0-9.]*%' | tail -1 | tr -d '%')
    ratios+=("$ratio")
  done

  avg=$(printf '%s\n' "${ratios[@]}" | awk '{s+=$1} END {printf "%.2f", s/NR}')
  echo "${file_type} ${size} data_4 average: ${avg}%" >> ratios.txt
}

run_bench() {
  local size="$1"
  local file_type="$2"
  local data_set="$3"
  local query="$4"

  if [[ $data_set -ne 4 ]]; then
    local file_path="bench_data/tmp_compressed/${file_type}/${size}_data/data_${data_set}/${data_set}_${size}.lz4"
    for i in $(seq 1 15); do
      gtime -a -o times.txt -f "%e" bash -c "lz4 -dc '${file_path}' | grep -c '${query}' > /dev/null || true"
    done
  else
    local file_path="bench_data/tmp_compressed/${file_type}/${size}_data/data_4"
    for i in $(seq 1 15); do
      gtime -a -o times.txt -f "%e" bash -c "for f in '${file_path}'/4_${size}_{1..4}.lz4; do lz4 -dc \"\$f\"; done | grep -c '${query}' > /dev/null || true"
    done
  fi

  echo "Data_${data_set} ${size} Average Time: $(awk '{s+=$1} END {print s/NR}' times.txt) seconds" >> bench_output.txt
  rm times.txt
}

rm -f ratios.txt
rm -f times.txt
rm -f bench_output.txt

setup_compress_data

queries=(",Male," "1994-01-01," "Pavel" "2025-09-30 ")
for data_set in {1..4}; do 
  for size in large medium small; do
      run_bench "${size}" "csv" "${data_set}" "${queries[data_set-1]}"
  done
done

rm -rf bench_data/tmp_compressed