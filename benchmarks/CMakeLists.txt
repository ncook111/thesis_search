add_executable(indexer_benchmarks
    bench_creation.cpp
    bench_search.cpp
    bench_encoding.cpp
    cache_flush.cpp
)

target_compile_definitions(indexer_benchmarks PRIVATE
    BENCH_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/bench_data"
)

target_link_libraries(indexer_benchmarks PRIVATE
    indexer_lib
    benchmark::benchmark_main
)