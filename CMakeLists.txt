cmake_minimum_required(VERSION 3.16)
project(ingest CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
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

# target
add_executable(ingest
    src/ingest.cpp
    src/coder.cpp
    src/tokenizer.cpp
    src/fst.cpp
    src/index.cpp
    src/helper.cpp
    src/pef.cpp
    src/search.cpp
    src/output.cpp
    third_party/simdjson/simdjson.cpp
)

target_include_directories(ingest PRIVATE
    include
    ${OPENFST_INCLUDE_DIR}
    third_party/ds2i
    third_party/simdjson
)

target_link_libraries(ingest PRIVATE
    succinct
    csv       
    Boost::program_options
    ${OPENFST_LIBRARY}
)