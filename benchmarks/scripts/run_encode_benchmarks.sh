#!/usr/bin/env bash
reps=${1:-1000}

mkdir -p ../results/encodings

./../../build/benchmarks/indexer_benchmarks --benchmark_filter=cenc --benchmark_out=../results/encodings/compression_encoding.json --benchmark_out_format=json --benchmark_report_aggregates_only=true --benchmark_enable_random_interleaving=true --benchmark_repetitions="$reps"
./../../build/benchmarks/indexer_benchmarks --benchmark_filter=cdec --benchmark_out=../results/encodings/compression_decoding.json --benchmark_out_format=json --benchmark_report_aggregates_only=true --benchmark_enable_random_interleaving=true --benchmark_repetitions="$reps"
./../../build/benchmarks/indexer_benchmarks --benchmark_filter=denc --benchmark_out=../results/encodings/delta_encoding.json --benchmark_out_format=json --benchmark_report_aggregates_only=true --benchmark_enable_random_interleaving=true --benchmark_repetitions="$reps"
./../../build/benchmarks/indexer_benchmarks --benchmark_filter=ddec --benchmark_out=../results/encodings/delta_decoding.json --benchmark_out_format=json --benchmark_report_aggregates_only=true --benchmark_enable_random_interleaving=true --benchmark_repetitions="$reps"