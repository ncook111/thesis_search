add_executable(indexer_benchmarks
    # bench_create.cpp
    bench_search_counts.cpp
    # bench_search_lines.cpp
    # bench_encoding.cpp
    cache_flush.cpp
)

target_link_libraries(indexer_benchmarks PRIVATE
    indexer_lib
    benchmark::benchmark
    benchmark::benchmark_main
)