cmake_minimum_required(VERSION 3.16)

include(FetchContent)

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

###### SIMD TARGET ######

set(SIMD_TARGET "scalar" CACHE STRING "SIMD target: scalar, neon, sse, avx2, avx512bw")
set_property(CACHE SIMD_TARGET PROPERTY STRINGS scalar neon sse avx2 avx512bw)

set(_simd ${SIMD_TARGET})

# check combination possible
if (_simd STREQUAL "neon" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    message(FATAL_ERROR "SIMD_TARGET=neon requested but target is not arm64/aarch64")
endif()
if (_simd MATCHES "^(sse|avx2|avx512bw)$" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    message(FATAL_ERROR "SIMD_TARGET=${_simd} requested but target is not x86_64")
endif()

add_library(simd_target INTERFACE)

if (_simd STREQUAL "scalar")
    target_compile_definitions(simd_target INTERFACE SIMD_TARGET_SCALAR=1)
elseif (_simd STREQUAL "neon")
    target_compile_options(simd_target INTERFACE -march=armv8-a+simd)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(simd_target INTERFACE -flax-vector-conversions)
    endif()
    target_compile_definitions(simd_target INTERFACE SIMD_TARGET_NEON=1)
elseif (_simd STREQUAL "sse")
    target_compile_options(simd_target INTERFACE -msse4.2)
    target_compile_definitions(simd_target INTERFACE SIMD_TARGET_SSE=1)
elseif (_simd STREQUAL "avx2")
    target_compile_options(simd_target INTERFACE -mavx2)
    target_compile_definitions(simd_target INTERFACE SIMD_TARGET_AVX2=1)
elseif (_simd STREQUAL "avx512bw")
    target_compile_options(simd_target INTERFACE -mavx512bw -mavx512f)
    target_compile_definitions(simd_target INTERFACE SIMD_TARGET_AVX512BW=1)
endif()

###### FASTLANES ######

FetchContent_Declare(
    fastlanes_src
    GIT_REPOSITORY https://github.com/cwida/FastLanes.git
    GIT_TAG        dev
)
FetchContent_Populate(fastlanes_src)

set(FASTLANES_PUB ${fastlanes_src_SOURCE_DIR}/publications/data_parallelized_encodings)

add_library(fastlanes_bp STATIC
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_ffor_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_unffor_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_rsum_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_unrsum_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_transpose_src.cpp
    ${FASTLANES_PUB}/fls_generated/fallback/scalar_av_uf1/fallback_scalar_av_1024_uf1_untranspose_src.cpp
)

target_include_directories(fastlanes_bp PUBLIC ${FASTLANES_PUB}/fls_generated/include)
target_link_libraries(fastlanes_bp PUBLIC simd_target)

if (_simd STREQUAL "neon")
    target_sources(fastlanes_bp PRIVATE
        ${FASTLANES_PUB}/fls_generated/arm64v8/neon_intrinsic_uf1/arm64v8_neon_intrinsic_1024_uf1_unffor_src.cpp
        ${FASTLANES_PUB}/fls_generated/arm64v8/neon_intrinsic_uf1/arm64v8_neon_intrinsic_1024_uf1_rsum_src.cpp)
elseif (_simd STREQUAL "sse")
    target_sources(fastlanes_bp PRIVATE
        ${FASTLANES_PUB}/fls_generated/x86_64/sse_intrinsic_uf1/x86_64_sse_intrinsic_1024_uf1_unffor_src.cpp)
elseif (_simd STREQUAL "avx2")
    target_sources(fastlanes_bp PRIVATE
        ${FASTLANES_PUB}/fls_generated/x86_64/avx2_intrinsic_uf1/x86_64_avx2_intrinsic_1024_uf1_unffor_src.cpp
        ${FASTLANES_PUB}/fls_generated/x86_64/avx2_intrinsic_uf1/x86_64_avx2_intrinsic_1024_uf1_rsum_src.cpp)
elseif (_simd STREQUAL "avx512bw")
    target_sources(fastlanes_bp PRIVATE
        ${FASTLANES_PUB}/fls_generated/x86_64/avx512bw_intrinsic_uf1/x86_64_avx512bw_intrinsic_1024_uf1_unffor_src.cpp
        ${FASTLANES_PUB}/fls_generated/x86_64/avx512bw_intrinsic_uf1/x86_64_avx512bw_intrinsic_1024_uf1_rsum_src.cpp)
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