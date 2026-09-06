#include "../src/ingest.h"
#include "../src/helper.h"
#include "fuzzing.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

static std::string index_path = "../../tests/test_data/all_indexes/index_1";
static std::string data_path = "../../tests/test_data/all_input_data";

class fuzzed_program_test_csv : public ::testing::Test {
protected:
    void SetUp() {
        generateFuzzedDataFile(FileType::CSV, 100000, data_path+"/fuzzed_csv", {"dob", "acc_num", "acc_bal"});
    }
    void TearDown() {
        std::filesystem::remove_all(data_path+"/fuzzed_csv");
        std::filesystem::remove_all(index_path+"_gvb");
        std::filesystem::remove_all(index_path+"_pef");
    }
};

class fuzzed_program_test_json : public ::testing::Test {
protected:
    void SetUp() {
        generateFuzzedDataFile(FileType::JSON, 100000, data_path+"/fuzzed_json", {"dob", "acc_num", "acc_bal"});
    }
    void TearDown() {
        std::filesystem::remove_all(data_path+"/fuzzed_json");
        std::filesystem::remove_all(index_path+"_gvb");
        std::filesystem::remove_all(index_path+"_pef");
    }
};

class fuzzed_program_test_logfmt : public ::testing::Test {
protected:
    void SetUp() {
        generateFuzzedDataFile(FileType::LOGFMT, 100000, data_path+"/fuzzed_logfmt", {"dob", "acc_num", "acc_bal"});
    }
    void TearDown() {
        std::filesystem::remove_all(data_path+"/fuzzed_logfmt");
        std::filesystem::remove_all(index_path+"_gvb");
        std::filesystem::remove_all(index_path+"_pef");
    }
};

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_pef", data_path+"/fuzzed_csv", {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_pef", data_path+"/fuzzed_csv", "", query, EncType::PEF));
    ASSERT_EQ(ret, 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_pef", data_path+"/fuzzed_json", {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_pef", data_path+"/fuzzed_json", "", query, EncType::PEF));
    ASSERT_EQ(ret, 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_pef", data_path+"/fuzzed_logfmt", {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string query = "[*] = \"here\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_pef", data_path+"/fuzzed_logfmt", "", query, EncType::PEF));
    ASSERT_EQ(ret, 0);
}