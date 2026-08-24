#include "../src/tokenizer.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "testing_setup.h"

TEST(alpha_test, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/alpha_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 6);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(alpha_test, test2) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/alpha_test_2.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 10);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 10);
}

TEST(float_test, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/float_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 4);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(num_test, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/num_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 5);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(date_test, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/date_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 2);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(time_test, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/time_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 3);
    ASSERT_EQ(tokens.getIPTokens().size(), 0);

    ASSERT_EQ(tokens.fields.size(), 1);
}

TEST(ip_test, test1) {
    std::string data_path = "../../tests/test_data/all_input_data/tokenizer_test_data/ipv4_test_1.csv";
    const Tokenizer tokens = Tokenizer::from_file(data_path, FileType::CSV, std::vector<std::string>()); 
    ASSERT_EQ(tokens.getAlphaTokens().size(), 0);
    ASSERT_EQ(tokens.getFloatTokens().size(), 0);
    ASSERT_EQ(tokens.getNumTokens().size(), 0);
    ASSERT_EQ(tokens.getDateTokens().size(), 0);
    ASSERT_EQ(tokens.getTimeTokens().size(), 0);
    ASSERT_EQ(tokens.getIPTokens().size(), 4);

    ASSERT_EQ(tokens.fields.size(), 1);
}