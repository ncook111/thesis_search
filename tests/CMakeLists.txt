add_executable(indexer_tests
    test_ast.cpp
    test_coder.cpp
    test_fst.cpp
    test_helper.cpp
    test_index.cpp
    test_ingest.cpp
    test_output.cpp
    test_parser.cpp
    test_pef.cpp
    test_search.cpp
    test_tokenizer.cpp
)

target_link_libraries(indexer_tests PRIVATE
    indexer_lib
    GTest::gtest_main
    GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(indexer_tests)