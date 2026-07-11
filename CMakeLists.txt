cmake_minimum_required(VERSION 3.16)
project(ingest CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()

list(PREPEND CMAKE_PREFIX_PATH /opt/homebrew)

# --- dependencies ---
find_package(Boost REQUIRED)

find_path(OPENFST_INCLUDE_DIR fst/fstlib.h)
find_library(OPENFST_LIBRARY fst)

if(NOT OPENFST_INCLUDE_DIR OR NOT OPENFST_LIBRARY)
    message(FATAL_ERROR "OpenFST not found — check `brew install openfst`")
endif()

# succinct has its own CMakeLists.txt that generates succinct_config.hpp
# via configure_file() — must be added as a subdirectory, not just an
# include path, or that header will never be generated.
add_subdirectory(third_party/ds2i/succinct)

# --- target ---
add_executable(ingest
    ingest.cpp
    coder.cpp
    tokenizer.cpp
    fst.cpp
    index.cpp
    helper.cpp
    pef.cpp
)

target_include_directories(ingest PRIVATE
    include
    ${OPENFST_INCLUDE_DIR}
    third_party/ds2i
)

target_link_libraries(ingest PRIVATE
    succinct
    Boost::boost
    ${OPENFST_LIBRARY}
)