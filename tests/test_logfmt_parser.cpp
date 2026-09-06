#include "../src/logfmt_parser.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

static std::string data_path = "../../tests/test_data/all_input_data";

TEST(invalid_file_test, test1) {
    ASSERT_THROW(LogfmtParser("invalid/path"), std::exception);
}

TEST(empty_file_test, test1) {
    LogfmtParser parser;
    ASSERT_NO_THROW(parser = LogfmtParser(data_path + "/logfmt/empty.logfmt"));

    ASSERT_TRUE(parser.current_row().empty());
    ASSERT_EQ(parser.current_offset(), -1);
    ASSERT_EQ(parser.begin(), parser.end());

}

TEST(small_simple_file_test, test1) {
    LogfmtParser parser;
    ASSERT_NO_THROW(parser = LogfmtParser(data_path + "/logfmt/small_simple.logfmt"));

    ASSERT_TRUE(!parser.current_row().empty());
    ASSERT_EQ(parser.current_offset(), 0);
    ASSERT_NE(parser.begin(), parser.end());

    int line_count = 0;
    for (auto it = parser.begin(); it != parser.end(); ++it) {
        line_count++;
        ASSERT_TRUE(!(*it).current_row().empty());
        ASSERT_NE((*it).current_offset(), -1);
    }

    ASSERT_EQ(line_count, 8);
}

TEST(small_complex_file_test, test1) {
    LogfmtParser parser;
    ASSERT_NO_THROW(parser = LogfmtParser(data_path + "/logfmt/small_complex.logfmt"));

    ASSERT_TRUE(!parser.current_row().empty());
    ASSERT_EQ(parser.current_offset(), 0);
    ASSERT_NE(parser.begin(), parser.end());

    int line_count = 0;
    for (auto it = parser.begin(); it != parser.end(); ++it) {
        line_count++;
        ASSERT_TRUE(!(*it).current_row().empty());
        ASSERT_NE((*it).current_offset(), -1);
    }

    ASSERT_EQ(line_count, 8);
}

// Large enough to trigger a rebuffer
TEST(long_simple_file_test, test1) {
    LogfmtParser parser;
    ASSERT_NO_THROW(parser = LogfmtParser(data_path + "/logfmt/large_simple.logfmt"));

    ASSERT_TRUE(!parser.current_row().empty());
    ASSERT_EQ(parser.current_offset(), 0);
    ASSERT_NE(parser.begin(), parser.end());

    int line_count = 0;
    for (auto it = parser.begin(); it != parser.end(); ++it) {
        line_count++;
        ASSERT_TRUE(!(*it).current_row().empty());
        ASSERT_NE((*it).current_offset(), -1);
    }

    ASSERT_EQ(line_count, 280);
}

// Large enough to trigger a rebuffer
TEST(long_complex_file_test, test1) {
        LogfmtParser parser;
    ASSERT_NO_THROW(parser = LogfmtParser(data_path + "/logfmt/large_complex.logfmt"));

    ASSERT_TRUE(!parser.current_row().empty());
    ASSERT_EQ(parser.current_offset(), 0);
    ASSERT_NE(parser.begin(), parser.end());

    int line_count = 0;
    for (auto it = parser.begin(); it != parser.end(); ++it) {
        line_count++;
        ASSERT_TRUE(!(*it).current_row().empty());
        ASSERT_NE((*it).current_offset(), -1);
    }

    ASSERT_EQ(line_count, 280);
}