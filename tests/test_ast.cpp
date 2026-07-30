#include <gtest/gtest.h>
#include "../src/query/ast.h"

/* test get_field */
TEST(get_field_test, test1) {
    std::string in = "table";
    std::vector<std::string> out = get_fields(in);
    ASSERT_EQ(out.size(), 1);
    ASSERT_EQ(out[0], "table");
}

TEST(get_field_test, test2) {
    std::string in = "table,chair";
    std::vector<std::string> out = get_fields(in);
    ASSERT_EQ(out.size(), 2);
    ASSERT_EQ(out[0], "table");
    ASSERT_EQ(out[1], "chair");
}

TEST(get_field_test, test3) {
    std::string in = ",table,chair,";
    std::vector<std::string> out = get_fields(in);
    ASSERT_EQ(out.size(), 2);
    ASSERT_EQ(out[0], "table");
    ASSERT_EQ(out[1], "chair");
}

TEST(get_field_test, test4) {
    std::string in = ",,,,,table,,,,,,,chair,,,,,,";
    std::vector<std::string> out = get_fields(in);
    ASSERT_EQ(out.size(), 2);
    ASSERT_EQ(out[0], "table");
    ASSERT_EQ(out[1], "chair");
}

TEST(get_field_test, test5) {
    std::string in = "table,chair,bedroom,cupboard,table,chair";
    std::vector<std::string> out = get_fields(in);
    ASSERT_EQ(out.size(), 6);
    ASSERT_EQ(out[0], "table");
    ASSERT_EQ(out[1], "chair");
    ASSERT_EQ(out[2], "bedroom");
    ASSERT_EQ(out[3], "cupboard");
    ASSERT_EQ(out[4], "table");
    ASSERT_EQ(out[5], "chair");
}

/* Test build_query */
TEST(build_query_test, test1) {
    std::vector<std::pair<uint8_t, std::string>> in_tokens;
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::GVB;
    std::unique_ptr<ASTNode> out = build_query(in_tokens, index_path, enc_type);
}