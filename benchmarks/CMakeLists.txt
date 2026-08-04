add_executable(indexer_benchmarks
    bench_program.cpp
)

target_link_libraries(indexer_benchmarks PRIVATE
    indexer_lib
    benchmark::benchmark
    benchmark::benchmark_main   # gives you main() for free
)