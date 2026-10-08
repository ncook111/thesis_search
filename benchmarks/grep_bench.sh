#!/bin/bash

######## Compress Data ########
setup_compress_data() {
  mkdir -p bench_data/tmp_compressed/{csv,json,logfmt}/{large,medium,small}_data/data_{1..5}

  # csv
  compress_loop "large" "csv"
  compress_loop "medium" "csv"
  compress_loop "small" "csv"

  # json
  compress_loop "large" "json"
  compress_loop "medium" "json"
  compress_loop "small" "json"

  # # logfmt
  compress_loop "large" "logfmt"
  compress_loop "medium" "logfmt"
  compress_loop "small" "logfmt"
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

  local ratios=()
  for i in {1..3}; do
    ratio=$(lz4 -f "${in}/data_5/5_${size}_${i}.${file_type}" \
                  "${out}/data_5/5_${size}_${i}.lz4" 2>&1 | grep -o '[0-9.]*%' | tail -1 | tr -d '%')
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

  if [[ $data_set -lt 4 ]]; then
    local file_path="bench_data/tmp_compressed/${file_type}/${size}_data/data_${data_set}/${data_set}_${size}.lz4"
    echo "Data_${data_set} ${size} ${file_type}" >> matches.txt
    for i in $(seq 1 20); do
      gtime -a -o times.txt -f "%e" bash -c "lz4 -dc '${file_path}' | grep -c '${query}' >> matches.txt || true"
    done
  elif [[ $data_set -eq 4 ]]; then
    local file_path="bench_data/tmp_compressed/${file_type}/${size}_data/data_4"
    echo "Data_${data_set} ${size} ${file_type}" >> matches.txt
    echo ${file_path} >> matches.txt
    for i in $(seq 1 20); do
      gtime -a -o times.txt -f "%e" bash -c "for f in '${file_path}'/4_${size}_{1..4}.lz4; do lz4 -dc \"\$f\"; done | grep -c '${query}' >> matches.txt || true"
    done
  elif [[ $data_set -eq 5 ]]; then
    local file_path="bench_data/tmp_compressed/${file_type}/${size}_data/data_5"
    echo "Data_${data_set} ${size} ${file_type}" >> matches.txt

    # Count occurances for this dataset as a csv document can be multiline in this instance, makes for a more fair comparison between csv/json/logfmt
    for i in $(seq 1 20); do
      gtime -a -o times.txt -f "%e" bash -c "for f in '${file_path}'/5_${size}_{1..3}.lz4; do lz4 -dc \"\$f\"; done | grep -o '${query}' | wc -l >> matches.txt || true"
    done
  fi

  echo "Data_${data_set} ${size} ${file_type} Average Time: $(awk '{s+=$1} END {print s/NR}' times.txt) seconds" >> bench_output.txt
  rm times.txt
}

rm -f ratios.txt
rm -f times.txt
rm -f bench_output.txt
rm -rf matches.txt

# setup_compress_data

queries=("Male" "1994-01-01" "Pavel" "2025-09-30" "class")
for data_set in {1..5}; do 
  for size in medium; do
    for file_type in csv json logfmt; do
      run_bench "${size}" "${file_type}" "${data_set}" "${queries[data_set-1]}"
    done
  done
done

# rm -rf bench_data/tmp_compressed