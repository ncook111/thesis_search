#include "../src/ingest.h"
#include "../src/helper.h"
#include "fuzzing.h"
#include "helper.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

static std::string index_path = "../../tests/test_data/all_indexes/";
static std::string data_path = "../../tests/test_data/all_input_data";
static std::string output_path = "../../tests/test_data/tmp.out";

class program_test : public ::testing::Test {
protected:
    void TearDown() {
        std::filesystem::remove_all(index_path+"index_gvb");
        std::filesystem::remove_all(index_path+"index_pef");
        std::filesystem::remove_all(index_path+"index_fls");
        std::filesystem::remove_all(index_path+"index_bpd1");
        std::filesystem::remove_all(index_path+"index_bpd2");
        std::filesystem::remove_all(index_path+"index_bpd4");
        std::filesystem::remove_all(index_path+"index_bpdm");
        std::filesystem::remove_all(output_path);
    }
};

TEST_F(program_test, creation_and_search_test_gvb) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"index_gvb", data_path+"/data_1", {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"index_gvb", "", output_path, query, EncType::GVB));
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(get_count_from_file(output_path), 4);
}

TEST_F(program_test, creation_and_search_test_pef) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"index_pef", data_path+"/data_1", {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"index_pef", "", output_path, query, EncType::PEF));
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(get_count_from_file(output_path), 4);
}

TEST_F(program_test, creation_and_search_test_fls) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"index_fls", data_path+"/data_1", {}, EncType::FLS));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"index_fls", "", output_path, query, EncType::FLS));
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(get_count_from_file(output_path), 4);
}

TEST_F(program_test, creation_and_search_test_bpd1) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"index_bpd1", data_path+"/data_1", {}, EncType::BPD1));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"index_bpd1", "", output_path, query, EncType::BPD1));
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(get_count_from_file(output_path), 4);
}

TEST_F(program_test, creation_and_search_test_bpd2) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"index_bpd2", data_path+"/data_1", {}, EncType::BPD2));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"index_bpd2", "", output_path, query, EncType::BPD2));
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(get_count_from_file(output_path), 4);
}

TEST_F(program_test, creation_and_search_test_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"index_bpd4", data_path+"/data_1", {}, EncType::BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"index_bpd4", "", output_path, query, EncType::BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(get_count_from_file(output_path), 4);
}

TEST_F(program_test, creation_and_search_test_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"index_bpdm", data_path+"/data_1", {}, EncType::BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"index_bpdm", "", output_path, query, EncType::BPDM));
    ASSERT_EQ(ret, 0);
     ASSERT_EQ(get_count_from_file(output_path), 4);
}