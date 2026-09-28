#include "../src/ingest.h"
#include "../src/helper.h"
#include "helper.h"
#include "fuzzing.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

static std::string index_path = "../../tests/test_data/all_indexes/index_1";
static std::string data_path = "../../tests/test_data/all_input_data";
static std::string output_path = "../../tests/test_data/tmp.out";

class fuzzed_program_test_csv : public ::testing::Test {
protected:
    void SetUp() {
        generateFuzzedDataFile(FileType::CSV, 100000, data_path+"/fuzzed_csv", {"dob", "acc_num", "acc_bal"});
    }
    void TearDown() {
        std::filesystem::remove_all(data_path+"/fuzzed_csv");
        std::filesystem::remove_all(output_path);

        for (const auto &entry : std::filesystem::directory_iterator(index_path)) {
            std::filesystem::remove_all(entry.path());
        }
    }
};

class fuzzed_program_test_json : public ::testing::Test {
protected:
    void SetUp() {
        generateFuzzedDataFile(FileType::JSON, 100000, data_path+"/fuzzed_json", {"dob", "acc_num", "acc_bal"});
    }
    void TearDown() {
        std::filesystem::remove_all(data_path+"/fuzzed_json");
        std::filesystem::remove_all(output_path);

        for (const auto &entry : std::filesystem::directory_iterator(index_path)) {
            std::filesystem::remove_all(entry.path());
        }
    }
};

class fuzzed_program_test_logfmt : public ::testing::Test {
protected:
    void SetUp() {
        generateFuzzedDataFile(FileType::LOGFMT, 100000, data_path+"/fuzzed_logfmt", {"dob", "acc_num", "acc_bal"});
    }
    void TearDown() {
        std::filesystem::remove_all(data_path+"/fuzzed_logfmt");
        std::filesystem::remove_all(output_path);

        for (const auto &entry : std::filesystem::directory_iterator(index_path)) {
            std::filesystem::remove_all(entry.path());
        }
    }
};

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_gvb) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_gvb", data_path+"/fuzzed_csv", {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::cout << "key: " + key << std::endl;
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_gvb", "", output_path, query, EncType::GVB));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_gvb) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_gvb", data_path+"/fuzzed_json", {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::cout << "key: " + key << std::endl;
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_gvb", "", output_path, query, EncType::GVB));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_gvb) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_gvb", data_path+"/fuzzed_logfmt", {}, EncType::GVB));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::cout << "key: " + key << std::endl;
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_gvb", "", output_path, query, EncType::GVB));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_pef) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_pef", data_path+"/fuzzed_csv", {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_pef", "", output_path, query, EncType::PEF));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_pef) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_pef", data_path+"/fuzzed_json", {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_pef", "", output_path, query, EncType::PEF));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_pef) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_pef", data_path+"/fuzzed_logfmt", {}, EncType::PEF));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_pef", "", output_path, query, EncType::PEF));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_fls) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_fls", data_path+"/fuzzed_csv", {}, EncType::FLS));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_fls", "", output_path, query, EncType::FLS));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_fls) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_fls", data_path+"/fuzzed_json", {}, EncType::FLS));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_fls", "", output_path, query, EncType::FLS));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_fls) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_fls", data_path+"/fuzzed_logfmt", {}, EncType::FLS));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_fls", "", output_path, query, EncType::FLS));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_s4_bpd1) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd1", data_path+"/fuzzed_csv", {}, EncType::S4_BPD1));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd1", "", output_path, query, EncType::S4_BPD1));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_s4_bpd1) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd1", data_path+"/fuzzed_json", {}, EncType::S4_BPD1));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd1", "", output_path, query, EncType::S4_BPD1));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_bpd1) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd1", data_path+"/fuzzed_logfmt", {}, EncType::S4_BPD1));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd1", "", output_path, query, EncType::S4_BPD1));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_s4_bpd2) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd2", data_path+"/fuzzed_csv", {}, EncType::S4_BPD2));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd2", "", output_path, query, EncType::S4_BPD2));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_s4_bpd2) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd2", data_path+"/fuzzed_json", {}, EncType::S4_BPD2));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd2", "", output_path, query, EncType::S4_BPD2));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_s4_bpd2) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd2", data_path+"/fuzzed_logfmt", {}, EncType::S4_BPD2));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd2", "", output_path, query, EncType::S4_BPD2));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_s4_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd4", data_path+"/fuzzed_csv", {}, EncType::S4_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd4", "", output_path, query, EncType::S4_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_s4_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd4", data_path+"/fuzzed_json", {}, EncType::S4_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd4", "", output_path, query, EncType::S4_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_s4_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpd4", data_path+"/fuzzed_logfmt", {}, EncType::S4_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpd4", "", output_path, query, EncType::S4_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_s4_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpdm", data_path+"/fuzzed_csv", {}, EncType::S4_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpdm", "", output_path, query, EncType::S4_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_s4_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpdm", data_path+"/fuzzed_json", {}, EncType::S4_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpdm", "", output_path, query, EncType::S4_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_s4_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s4_bpdm", data_path+"/fuzzed_logfmt", {}, EncType::S4_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s4_bpdm", "", output_path, query, EncType::S4_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}
#endif

#if defined(SIMD_TARGET_AVX2)
TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_s8_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s8_bpd4", data_path+"/fuzzed_csv", {}, EncType::S8_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s8_bpd4", "", output_path, query, EncType::S4_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_s8_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s8_bpd4", data_path+"/fuzzed_json", {}, EncType::S8_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s8_bpd4", "", output_path, query, EncType::S8_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_s8_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s8_bpd4", data_path+"/fuzzed_logfmt", {}, EncType::S8_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s8_bpd4", "", output_path, query, EncType::S8_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_s8_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s8_bpdm", data_path+"/fuzzed_csv", {}, EncType::S8_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s8_bpdm", "", output_path, query, EncType::S8_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_s8_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s8_bpdm", data_path+"/fuzzed_json", {}, EncType::S8_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s8_bpdm", "", output_path, query, EncType::S8_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_s8_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s8_bpdm", data_path+"/fuzzed_logfmt", {}, EncType::S8_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s8_bpdm", "", output_path, query, EncType::S8_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}
#endif

#if defined(SIMD_TARGET_AVX512BW)
TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_s16_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s16_bpd4", data_path+"/fuzzed_csv", {}, EncType::S16_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s16_bpd4", "", output_path, query, EncType::S4_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_s16_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s16_bpd4", data_path+"/fuzzed_json", {}, EncType::S16_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s16_bpd4", "", output_path, query, EncType::S16_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_s16_bpd4) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s16_bpd4", data_path+"/fuzzed_logfmt", {}, EncType::S16_BPD4));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s16_bpd4", "", output_path, query, EncType::S16_BPD4));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_csv, creation_and_search_test_fuzzed_csv_s16_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s16_bpdm", data_path+"/fuzzed_csv", {}, EncType::S16_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_csv/fuzzed.csv", FileType::CSV);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s16_bpdm", "", output_path, query, EncType::S16_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_json, creation_and_search_test_fuzzed_json_s16_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s16_bpdm", data_path+"/fuzzed_json", {}, EncType::S16_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_json/fuzzed.json", FileType::JSON);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s16_bpdm", "", output_path, query, EncType::S16_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}

TEST_F(fuzzed_program_test_logfmt, creation_and_search_test_fuzzed_logfmt_s16_bpdm) {
    int ret = -1;
    ASSERT_NO_THROW(ret = create_mode(index_path+"_s16_bpdm", data_path+"/fuzzed_logfmt", {}, EncType::S16_BPDM));
    ASSERT_EQ(ret, 0);
    
    ret = -1;
    std::string key = get_matching_query(data_path+"/fuzzed_logfmt/fuzzed.logfmt", FileType::LOGFMT);
    std::string query = "[*] = \"" + key + "\"";
    ASSERT_NO_THROW(ret = search_mode(index_path+"_s16_bpdm", "", output_path, query, EncType::S16_BPDM));
    ASSERT_EQ(ret, 0);
    ASSERT_NE(get_count_from_file(output_path), 0);
}
#endif