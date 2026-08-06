add_executable(indexer_benchmarks
    bench_program.cpp
    bench_encoding.cpp
    cache_flush.cpp
)

target_link_libraries(indexer_benchmarks PRIVATE
    indexer_lib
    benchmark::benchmark
    benchmark::benchmark_main
)