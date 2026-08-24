#include "../src/ingest.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "testing_setup.h"

TEST(creation_and_search_test, test1) {
    std::string index_path = "../../tests/test_data/all_indexes/index_1";
    std::string data_path = "../../tests/test_data/all_input_data/data_1";

    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_gvb", data_path, {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_gvb", data_path, "", query, EncType::GVB));
    ASSERT_EQ(ret, 0);
}

TEST(creation_and_search_test, test2) {
    std::string index_path = "../../tests/test_data/all_indexes/index_1";
    std::string data_path = "../../tests/test_data/all_input_data/data_1";

    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_pef", data_path, {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_pef", data_path, "", query, EncType::PEF));
    ASSERT_EQ(ret, 0);
}