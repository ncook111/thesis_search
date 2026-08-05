cmake_minimum_required(VERSION 3.16)
project(indexer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

list(PREPEND CMAKE_PREFIX_PATH /opt/homebrew)

# dependencies
find_package(Boost REQUIRED COMPONENTS program_options)

find_path(OPENFST_INCLUDE_DIR fst/fstlib.h)
find_library(OPENFST_LIBRARY fst)

if(NOT OPENFST_INCLUDE_DIR OR NOT OPENFST_LIBRARY)
    message(FATAL_ERROR "OpenFST not found, check `brew install openfst`")
endif()

add_subdirectory(third_party/csv-parser)
add_subdirectory(third_party/ds2i/succinct)

add_library(indexer_lib
    src/ingest.cpp
    src/coder.cpp
    src/tokenizer.cpp
    src/fst.cpp
    src/index.cpp
    src/helper.cpp
    src/pef.cpp
    src/search.cpp
    src/output.cpp
    src/query/parser.cpp
    src/query/ast.cpp
    third_party/simdjson/simdjson.cpp
)

target_include_directories(indexer_lib PUBLIC
    include
    ${OPENFST_INCLUDE_DIR}
    third_party/ds2i
    third_party/simdjson
)

target_link_libraries(indexer_lib PUBLIC
    succinct
    csv
    Boost::program_options
    ${OPENFST_LIBRARY}
)

add_executable(indexer src/indexer.cpp)
target_link_libraries(indexer PRIVATE indexer_lib)

include(CTest)

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()

option(BUILD_BENCHMARKS "Build performance benchmarks" ON)

if(BUILD_BENCHMARKS)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1
    )
    FetchContent_MakeAvailable(googlebenchmark)

    add_subdirectory(benchmarks)
endif()