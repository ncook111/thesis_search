cmake_minimum_required(VERSION 3.28)

include(FetchContent)

project(indexer C CXX) # C needed for simdcomp

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

###### BOOST ######

set(BOOST_ENABLE_COMPATIBILITY_TARGETS ON)
set(BOOST_IOSTREAMS_ENABLE_ZLIB  OFF)
set(BOOST_IOSTREAMS_ENABLE_BZIP2 OFF)
set(BOOST_IOSTREAMS_ENABLE_LZMA  OFF)
set(BOOST_IOSTREAMS_ENABLE_ZSTD  OFF)

FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-cmake.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    EXCLUDE_FROM_ALL
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(Boost)

###### OPENFST ######

FetchContent_Declare(
    openfst_src
    GIT_REPOSITORY https://github.com/google-research/openfst.git
    GIT_TAG        08f1fb92b2ef7a1657d25d3ddd9db1d6aa866c66 # 1.8.5
)
FetchContent_MakeAvailable(openfst_src)

set(OPENFST_SRC ${openfst_src_SOURCE_DIR}/src)

add_library(openfst STATIC
    ${OPENFST_SRC}/lib/compat.cc
    ${OPENFST_SRC}/lib/encode.cc
    ${OPENFST_SRC}/lib/flags.cc
    ${OPENFST_SRC}/lib/fst.cc
    ${OPENFST_SRC}/lib/fst-types.cc
    ${OPENFST_SRC}/lib/mapped-file.cc
    ${OPENFST_SRC}/lib/properties.cc
    ${OPENFST_SRC}/lib/symbol-table.cc
    ${OPENFST_SRC}/lib/symbol-table-ops.cc
    ${OPENFST_SRC}/lib/weight.cc
    ${OPENFST_SRC}/lib/util.cc
)
target_include_directories(openfst SYSTEM PUBLIC ${OPENFST_SRC}/include)
target_compile_options(openfst PRIVATE
    -fsized-deallocation
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-defaulted-function-deleted>
)
target_link_libraries(openfst PUBLIC ${CMAKE_DL_LIBS})

###### SIMDJSON ######

FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        90adaa4aef486a1743dd888297c27cba64101f79 #
)
FetchContent_MakeAvailable(simdjson)

###### DS2I ######

FetchContent_Declare(
    ds2i_src
    GIT_REPOSITORY https://github.com/ncook111/ds2i_u64.git
    GIT_TAG        8a00f31448df4d7698d021b9be19eb0a2a4378c0
    GIT_SUBMODULES succinct
    SOURCE_SUBDIR  succinct
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ds2i_src)

target_link_libraries(succinct PUBLIC Boost::headers Boost::iostreams)

###### CSV-PARSER ######

FetchContent_Declare(
    csv_src
    GIT_REPOSITORY https://github.com/vincentlaucsb/csv-parser.git
    GIT_TAG        5976d7a44df00ac55a6feec28a5e0a11d06f1872
)
set(CSV_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(csv_src)

###### SIMD TARGET ######

set(SIMD_TARGET "avx512" CACHE STRING "SIMD target: scalar, neon, sse, avx2, avx512bw")
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
    target_compile_options(simd_target INTERFACE -fno-tree-vectorize -fno-tree-slp-vectorize)
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
    target_compile_definitions(simd_target INTERFACE SIMD_TARGET_AVX2=1 SIMD_TARGET_SSE=1)
elseif (_simd STREQUAL "avx512bw")
    target_compile_options(simd_target INTERFACE -mavx512bw -mavx512f)
    target_compile_definitions(simd_target INTERFACE SIMD_TARGET_AVX512BW=1 SIMD_TARGET_AVX2=1 SIMD_TARGET_SSE=1)
endif()

###### FASTLANES ######

FetchContent_Declare(
    fastlanes_src
    GIT_REPOSITORY https://github.com/cwida/FastLanes.git
    GIT_TAG        f0edc1020a538f1f8098640fce8347c9ac247a0d
    SOURCE_SUBDIR  _no_cmake_
)
FetchContent_MakeAvailable(fastlanes_src)

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

###### SIMDBP128 ######

FetchContent_Declare(
    simdcomp_src
    GIT_REPOSITORY https://github.com/fast-pack/SIMDCompressionAndIntersection.git
    GIT_TAG        v0.2.0
)
set(SIMDCOMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(simdcomp_src)

###### SIMDCOMP ######

if (_simd MATCHES "^(avx2|avx512bw)$")
    FetchContent_Declare(
        simdcomp_kernels_src
        GIT_REPOSITORY https://github.com/fast-pack/simdcomp.git
        GIT_TAG        d5301778fe5045ca8099252de5a6ed8016a287df
        SOURCE_SUBDIR  _no_cmake_
    )
    FetchContent_MakeAvailable(simdcomp_kernels_src)

    add_library(simdcomp_avx STATIC
        ${simdcomp_kernels_src_SOURCE_DIR}/src/avxbitpacking.c
        ${simdcomp_kernels_src_SOURCE_DIR}/src/avx512bitpacking.c
        ${simdcomp_kernels_src_SOURCE_DIR}/src/simdcomputil.c
    )

    target_include_directories(simdcomp_avx PRIVATE ${simdcomp_kernels_src_SOURCE_DIR}/include)

    target_link_libraries(simdcomp_avx PRIVATE simd_target)
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
    src/metadata.cpp
    src/simdbp128.cpp
    src/segment.cpp
    src/search.cpp
    src/output.cpp
    src/crc.cpp
    src/logfmt_parser.cpp
    src/query/parser.cpp
    src/query/ast.cpp
)

target_include_directories(indexer_lib PUBLIC
    include
    ${ds2i_src_SOURCE_DIR}
)

target_link_libraries(indexer_lib PUBLIC
    succinct
    csv
    Boost::program_options
    Boost::headers
    $<LINK_LIBRARY:WHOLE_ARCHIVE,openfst>
    fastlanes_bp
    SIMDCompressionAndIntersection::SIMDCompressionAndIntersection
    simdjson::simdjson
    simd_target
)

if (TARGET simdcomp_avx)
    target_link_libraries(indexer_lib PUBLIC simdcomp_avx)
endif()

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