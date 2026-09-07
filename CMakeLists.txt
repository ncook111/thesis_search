include(FetchContent)

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

FetchContent_Declare(
    fastlanes_src
    GIT_REPOSITORY https://github.com/cwida/FastLanes.git
    GIT_TAG        dev
)
FetchContent_Populate(fastlanes_src)

set(FASTLANES_PUB ${fastlanes_src_SOURCE_DIR}/publications/data_parallelized_encodings)

add_library(fastlanes_bp STATIC
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_pack_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_unpack_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_ffor_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_unffor_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_unrsum_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_rsum_src.cpp
)

target_include_directories(fastlanes_bp PUBLIC
    ${FASTLANES_PUB}/fls_generated/include
)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    target_sources(fastlanes_bp PRIVATE
        ${FASTLANES_PUB}/fls_generated/arm64v8/neon_intrinsic_uf1/arm64v8_neon_intrinsic_1024_uf1_unffor_src.cpp
        ${FASTLANES_PUB}/fls_generated/arm64v8/neon_intrinsic_uf1/arm64v8_neon_intrinsic_1024_uf1_unpack_src.cpp
        ${FASTLANES_PUB}/fls_generated/arm64v8/neon_intrinsic_uf1/arm64v8_neon_intrinsic_1024_uf1_rsum_src.cpp
    )
    target_compile_options(fastlanes_bp PUBLIC -march=armv8-a+simd)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(fastlanes_bp PRIVATE -flax-vector-conversions)
    endif()
    target_compile_definitions(fastlanes_bp PUBLIC FASTLANES_HAVE_NEON=1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    target_sources(fastlanes_bp PRIVATE
        ${FASTLANES_PUB}/fls_generated/x86_64/avx512bw_intrinsic_uf1/x86_64_avx512bw_intrinsic_1024_uf1_unffor_src.cpp
        ${FASTLANES_PUB}/fls_generated/x86_64/avx512bw_intrinsic_uf1/x86_64_avx512bw_intrinsic_1024_uf1_unpack_src.cpp
        ${FASTLANES_PUB}/fls_generated/x86_64/avx512bw_intrinsic_uf1/x86_64_avx512bw_intrinsic_1024_uf1_rsum_src.cpp
    )
    target_compile_options(fastlanes_bp PUBLIC -mavx512bw -mavx512f)
endif()

add_library(indexer_lib
    src/ingest.cpp
    src/coder.cpp
    src/tokenizer.cpp
    src/fst.cpp
    src/index.cpp
    src/helper.cpp
    src/pef.cpp
    src/fls.cpp
    src/search.cpp
    src/output.cpp
    src/crc.cpp
    src/logfmt_parser.cpp
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
    fastlanes_bp
)

add_executable(indexer src/indexer.cpp)
target_link_libraries(indexer PRIVATE indexer_lib)

include(CTest)

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTING)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    set(FAKER_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        faker
        GIT_REPOSITORY https://github.com/cieslarmichal/faker-cxx.git
        GIT_TAG        main
    )
    FetchContent_MakeAvailable(faker)

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