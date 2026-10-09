#include "../src/tokenizer.h"
#include "../src/helper.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "helper.h"

TEST(alpha_test_file_csv, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/alpha_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 6);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(alpha_test_file_csv, test2) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/alpha_test_2.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 10);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 10);
}

TEST(alpha_test_file_json, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/alpha_test_1.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 6);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(alpha_test_file_json, test2) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/alpha_test_2.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 10);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 10);
}

TEST(alpha_test_file_logfmt, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/alpha_test_1.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 6);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}


TEST(alpha_test_file_logfmt, test2) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/alpha_test_2.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 10);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 10);
}

TEST(float_test_file_csv, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/float_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 4);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(float_test_file_json, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/float_test_1.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 4);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(float_test_file_logfmt, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/float_test_1.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 4);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(pos_num_test_file_csv, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/num_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 5);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(pos_num_test_file_json, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/num_test_1.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 5);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(pos_num_test_file_logfmt, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/num_test_1.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 5);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(neg_num_test_file_csv, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/neg_num_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 5);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(neg_num_test_file_json, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/neg_num_test_1.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 5);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(neg_num_test_file_logfmt, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/neg_num_test_1.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 5);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(date_test_file_csv, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/date_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 2);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(date_test_file_json, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/date_test_1.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 2);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(date_test_file_logfmt, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/date_test_1.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 2);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(time_test_file_csv, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/time_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 3);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(time_test_file_json, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/time_test_1.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 3);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(time_test_file_logfmt, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/time_test_1.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 3);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(ip_test_file_csv, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/ipv4_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 4);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(ip_test_file_json, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/ipv4_test_1.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 4);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(ip_test_file_logfmt, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/ipv4_test_1.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 4);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(mixed_test_file_csv, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/csv/mixed_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 1);
    ASSERT_EQ(tokens.getFloatTokens().size(), 1);
    ASSERT_EQ(tokens.getNumTokens().size(), 1);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 1);
    ASSERT_EQ(tokens.getDateTokens().size(), 1);
    ASSERT_EQ(tokens.getTimeTokens().size(), 1);
    ASSERT_EQ(tokens.getIPTokens().size(), 1);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(mixed_test_file_json, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/json/mixed_test_1.json";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::JSON, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 1);
    ASSERT_EQ(tokens.getFloatTokens().size(), 1);
    ASSERT_EQ(tokens.getNumTokens().size(), 1);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 1);
    ASSERT_EQ(tokens.getDateTokens().size(), 1);
    ASSERT_EQ(tokens.getTimeTokens().size(), 1);
    ASSERT_EQ(tokens.getIPTokens().size(), 1);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(mixed_test_file_logfmt, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/logfmt/mixed_test_1.logfmt";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::LOGFMT, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 1);
    ASSERT_EQ(tokens.getFloatTokens().size(), 1);
    ASSERT_EQ(tokens.getNumTokens().size(), 1);
    ASSERT_EQ(tokens.getNegNumTokens().size(), 1);
    ASSERT_EQ(tokens.getDateTokens().size(), 1);
    ASSERT_EQ(tokens.getTimeTokens().size(), 1);
    ASSERT_EQ(tokens.getIPTokens().size(), 1);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(alpha_test_string, test1) {
    std::string input = "one two three four five six";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 6);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(float_test_string, test1) {
    std::string input = "1.1 5.555 9.0 0.11";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 0);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 4);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(pos_num_test_string, test1) {
    std::string input = "1 5 99999999 556545 0";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 0);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 5);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(neg_num_test_string, test1) {
    std::string input = "-1 -5 -99999999 -556545 -99";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 0);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 5);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(date_test_string, test1) {
    std::string input = "2001-03-23 1999-01-01 2033-12-31";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 0);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 3);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(time_test_string, test1) {
    std::string input = "14:30:00 12:01:00 00:00:00 00:30:50";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 0);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 4);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(ip_test_string, test1) {
    std::string input = "192.168.0.1 1.1.1.1";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 0);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 2);
}

TEST(mixed_test_string, test1) {
    std::string input = "hello 1 -1 1.1 2001-03-23 14:30:22 1.1.1.1";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 1);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 1);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 1);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 1);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 1);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 1);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 1);
}

TEST(large_pint_string, test1) {
    std::string input = "18446744073709551616";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 1);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(large_pint_string, test2) {
    std::string input = "18446744073709551615";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 1);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(large_pint_string, test3) {
    std::string input = "18446744073709551614";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 0);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 1);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(large_nint_string, test1) {
    std::string input = "-18446744073709551616";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 1);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(large_nint_string, test2) {
    std::string input = "-18446744073709551615";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 1);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}

TEST(large_nint_string, test3) {
    std::string input = "-18446744073709551614";
    const Tokenizer tokens = Tokenizer::from_string(input); 
    ASSERT_EQ(tokens.getAlphaTokensQ().size(), 0);
    ASSERT_EQ(tokens.getFloatTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNumTokensQ().size(), 0);
    ASSERT_EQ(tokens.getNegNumTokensQ().size(), 1);
    ASSERT_EQ(tokens.getDateTokensQ().size(), 0);
    ASSERT_EQ(tokens.getTimeTokensQ().size(), 0);
    ASSERT_EQ(tokens.getIPTokensQ().size(), 0);
}