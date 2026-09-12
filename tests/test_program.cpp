#include "../src/ingest.h"
#include "../src/helper.h"
#include "fuzzing.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

static std::string index_path = "../../tests/test_data/all_indexes/index_1";
static std::string data_path = "../../tests/test_data/all_input_data";

class program_test : public ::testing::Test {
protected:
    void TearDown() {
        std::filesystem::remove_all(index_path+"_gvb");
        std::filesystem::remove_all(index_path+"_pef");
        std::filesystem::remove_all(index_path+"_fls");
    }
};

TEST_F(program_test, creation_and_search_test_gvb) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_gvb", data_path+"/data_1", {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_gvb", data_path+"/data_1", "", query, EncType::GVB));
    ASSERT_EQ(ret, 0);
}

TEST_F(program_test, creation_and_search_test_pef) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_pef", data_path+"/data_1", {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_pef", data_path+"/data_1", "", query, EncType::PEF));
    ASSERT_EQ(ret, 0);
}

TEST_F(program_test, creation_and_search_test_fls) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_fls", data_path+"/data_1", {}, EncType::FLS));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_fls", data_path+"/data_1", "", query, EncType::FLS));
    ASSERT_EQ(ret, 0);
}