#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/query/ast.h"
#include "../src/query/parser.h"
#include "../src/ingest.h"
#include "testing_setup.h"
#include <filesystem>

// Test data
std::vector<std::string> invalid_range = {"{1 ,2}", "{1,2 }", "{ 1,2}", "{1, 2}", "{a,b}", "{a}", "{1}", "{1:}", "{1:2}", "{1,,2}"};
std::vector<std::string> valid_range = {"{1,2}", "{1,}", "{,2}", "{1:,}", "{1,:2}", "{1.554,2.33}", "{1.554,}", "{,2.33}"};

class parse_queries_basic_test : public index_setup {};
class parse_queries_compliment_test : public index_setup {};
class parse_queries_union_test : public index_setup {};
class parse_queries_intersection_test : public index_setup {};
class parse_queries_diff_test : public index_setup {};
class parse_queries_sym_diff_test : public index_setup {};

/* test is_range_query */
TEST(range_query_test, test1) {
    for (const auto rng : invalid_range) {
        EXPECT_FALSE(is_range_query(rng));
    }
}

TEST(range_query_test, test2) {
    for (const auto rng : valid_range) {
        EXPECT_TRUE(is_range_query(rng));
    }
}

/* test tokenize */
TEST(tokenize_test, test1) {
    std::string in = "[table] = \"chair\"";
    std::vector<std::pair<uint8_t, std::string>> out = tokenize(in);
    
    std::vector<std::pair<uint8_t, std::string>> expected = {{QueryTokType::STDQUERY, "chair"}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, "table"}};
    ASSERT_EQ(out, expected);
}

TEST(tokenize_test, test2) {
    std::string in = "[*] = \"chair\" | [table] = {1,2}";
    std::vector<std::pair<uint8_t, std::string>> out = tokenize(in);
    
    std::vector<std::pair<uint8_t, std::string>> expected = {{QueryTokType::RNGQUERY, "{1,2}"}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, "table"}, {QueryTokType::UNION, "|"}, {QueryTokType::STDQUERY, "chair"}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, "*"}};
    ASSERT_EQ(out, expected);
}

TEST(tokenize_test, test3) {
    std::string in = "[*] = \"chair\" | [table] = {1,2} & [table,next] = \"going to the shops\" - [*] = \"123\" / [field] = \"1.2\"";
    std::vector<std::pair<uint8_t, std::string>> out = tokenize(in);
    
    std::vector<std::pair<uint8_t, std::string>> expected = {{QueryTokType::STDQUERY, "1.2"}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, "field"}, {QueryTokType::SYMDIFF, "/"}, {QueryTokType::STDQUERY, "123"}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, "*"}, {QueryTokType::DIFF, "-"}, {QueryTokType::STDQUERY, "going to the shops"}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, "table,next"}, {QueryTokType::INTER, "&"}, {QueryTokType::RNGQUERY, "{1,2}"}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, "table"}, {QueryTokType::UNION, "|"}, {QueryTokType::STDQUERY, "chair"}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, "*"}};
    ASSERT_EQ(out, expected);
}

TEST(tokenize_test, test4) {
    std::string in = "| [*] = \"chair\"";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Query must begin with either a opening parenthesis or a field"))
    );
}

TEST(tokenize_test, test5) {
    std::string in = "[[*] = \"chair\"";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Misplaced '["))
    );
}

TEST(tokenize_test, test6) {
    std::string in = "[*]] = \"chair\"";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Misplaced ']'"))
    );
}

TEST(tokenize_test, test7) {
    std::string in = "[*][*] = \"chair\"";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Expected an equals sign after a field declaration. Received: *"))
    );
}

TEST(tokenize_test, test8) {
    std::string in = "[*] == \"chair\"";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Expected a standard or range query declaration after an equals sign. Received: ="))
    );
}

TEST(tokenize_test, test9) {
    std::string in = "[*] | \"chair\"";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Expected an equals sign after a field declaration. Received: |"))
    );
}

TEST(tokenize_test, test10) {
    std::string in = "[(*)] = \"chair\"";
    EXPECT_NO_THROW(tokenize(in));
}

TEST(tokenize_test, test11) {
    std::string in = "[*] = \"(chair)\"";
    EXPECT_NO_THROW(tokenize(in));
}

TEST(tokenize_test, test12) {
    std::string in = "[*] = {{1,2}";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Misplaced '{'"))
    );
}

TEST(tokenize_test, test13) {
    std::string in = "[*] = {1,2}}";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Misplaced '}'"))
    );
}

TEST(tokenize_test, test14) {
    for (const auto rng : invalid_range) {
        std::string in = "[*] = " + rng;
        EXPECT_THAT(
            [&]() { tokenize(in); },
            ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Invalid range query. Received: " + rng))
        );
    }
}

TEST(tokenize_test, test15) {
    std::string in = "[*,field2] = {1,2}";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Field cannot contain wild card and additional items"))
    );
}

TEST(tokenize_test, test16) {
    std::string in = "[*] = ){1,2}";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Expected a standard or range query declaration after an equals sign. Received: )"))
    );
}

TEST(tokenize_test, test17) {
    std::string in = "[*] = ({1,2}";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Expected a standard or range query declaration after an equals sign. Received: ("))
    );
}

TEST(tokenize_test, test18) {
    std::string in = "[*] (= {1,2}";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Expected an equals sign after a field declaration. Received: ("))
    );
}

TEST(tokenize_test, test19) {
    std::string in = "[*]) = {1,2}";
    EXPECT_THAT(
        [&]() { tokenize(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Expected an equals sign after a field declaration. Received: )"))
    );
}

/* test check_field */
TEST(check_field_test, test1) {
    std::string in = "table";
    ASSERT_NO_THROW(check_fields(in));
}

TEST(check_field_test, test2) {
    std::string in = "table,chair";
    ASSERT_NO_THROW(check_fields(in));
}

TEST(check_field_test, test3) {
    std::string in = ",table,chair,";
    EXPECT_THAT(
        [&]() { check_fields(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Field cannot contain empty items"))
    );
}

TEST(check_field_test, test4) {
    std::string in = ",,,,,table,,,,,,,chair,,,,,,";
    EXPECT_THAT(
        [&]() { check_fields(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Field cannot contain empty items"))
    );
}

TEST(check_field_test, test5) {
    std::string in = "table, ,chair";
    EXPECT_THAT(
        [&]() { check_fields(in); },
        ::testing::ThrowsMessage<std::invalid_argument>(::testing::HasSubstr("Field cannot contain empty items"))
    );
}

TEST(check_field_test, test6) {
    std::string in = "table,chair,bedroom,cupboard,table,chair";
    ASSERT_NO_THROW(check_fields(in));
}

/* test parse_queries */
TEST_F(parse_queries_basic_test, test1) {
    std::string in = "[*] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,2,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_basic_test, test2) {
    std::string in = "[*] = \"not\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_basic_test, test3) {
    std::string in = "[head] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,2};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_basic_test, test4) {
    std::string in = "[head] = \"not\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {1,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_basic_test, test5) {
    std::string in = "[tail] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {1,2,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_basic_test, test6) {
    std::string in = "[tail] = \"not\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_basic_test, test7) {
    std::string in = "[head,tail] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,2,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_basic_test, test8) {
    std::string in = "[head,tail] = \"not\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

// Compliment
TEST_F(parse_queries_compliment_test, test1) {
    std::string in = "[*] = \"here\" & ![*] = \"not\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {2};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_compliment_test, test2) {
    std::string in = "![*] = \"here\" & ![*] = \"not\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_compliment_test, test3) {
    std::string in = "![*] = \"here\" | ![*] = \"not\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {2};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_compliment_test, test4) {
    std::string in = "!([*] = \"here\" | [*] = \"not\")";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_compliment_test, test5) {
    std::string in = "[*] = \"here\" & !([*] = \"a\" | [*] = \"b\")";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,2,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_union_test, test1) {
    std::string in = "[*] = \"not\" | [*] = \"b\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_union_test, test2) {
    std::string in = "[*] = \"b\" | !([*] = \"c\")";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,2,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_union_test, test3) {
    std::string in = "!([*] = \"b\" | [tail] = \"not\")";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {1,2,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_intersection_test, test1) {
    std::string in = "[head] = \"not\" & [tail] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {1,3};
        ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_intersection_test, test2) {
    std::string in = "[head] = \"here\" & [tail] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {2};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_intersection_test, test3) {
    std::string in = "[head] = \"here\" & ![tail] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_diff_test, test1) {
    std::string in = "[*] = \"here\" - ![*] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,2,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_sym_diff_test, test1) {
    std::string in = "[*] = \"here\" / [*] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

TEST_F(parse_queries_sym_diff_test, test2) {
    std::string in = "[*] = \"here\" / ![*] = \"here\"";
    Metadata metadata_pef = Metadata::read("../../tests/test_data/all_indexes/index_1_pef");
    Metadata metadata_gvb = Metadata::read("../../tests/test_data/all_indexes/index_1_gvb");
    FieldMap out_pef = parse_queries(in, metadata_pef);
    FieldMap out_gvb = parse_queries(in, metadata_gvb);
    std::vector<uint64_t> expected = {0,1,2,3};
    ASSERT_EQ(out_pef.size(), 1);
    ASSERT_EQ(out_pef.at(0), expected);
    ASSERT_EQ(out_gvb.size(), 1);
    ASSERT_EQ(out_gvb.at(0), expected);
}

/* test get_next_token */
TEST(get_next_token_test, test1) {
    std::vector<std::pair<uint8_t, std::string>> in_tokens = {{QueryTokType::STDQUERY, "hello"}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, ""}, {QueryTokType::COMPL, "!"}, {QueryTokType::INVALID, ""}};
    ASSERT_EQ(get_next_token(in_tokens), std::make_pair(QueryTokType::INVALID, ""));
    ASSERT_EQ(get_next_token(in_tokens), std::make_pair(QueryTokType::COMPL, "!"));
    ASSERT_EQ(get_next_token(in_tokens), std::make_pair(QueryTokType::FIELD, ""));
    ASSERT_EQ(get_next_token(in_tokens), std::make_pair(QueryTokType::EQ, ""));
    ASSERT_EQ(get_next_token(in_tokens), std::make_pair(QueryTokType::STDQUERY, "hello"));
    EXPECT_THAT(
        [&]() { get_next_token(in_tokens); },
        ::testing::ThrowsMessage<std::runtime_error>(::testing::HasSubstr("Tokens list is empty"))
    );

}

/* test peak_next_token */
TEST(peak_next_token_test, test1) {
    std::vector<std::pair<uint8_t, std::string>> in_tokens = {{QueryTokType::STDQUERY, "hello"}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, ""}, {QueryTokType::COMPL, "!"}, {QueryTokType::INVALID, ""}};
    ASSERT_EQ(peak_next_token(in_tokens), std::make_pair(QueryTokType::INVALID, ""));
    in_tokens.pop_back();
    ASSERT_EQ(peak_next_token(in_tokens), std::make_pair(QueryTokType::COMPL, "!"));
    in_tokens.pop_back();
    ASSERT_EQ(peak_next_token(in_tokens), std::make_pair(QueryTokType::FIELD, ""));
    in_tokens.pop_back();
    ASSERT_EQ(peak_next_token(in_tokens), std::make_pair(QueryTokType::EQ, ""));
    in_tokens.pop_back();
    ASSERT_EQ(peak_next_token(in_tokens), std::make_pair(QueryTokType::STDQUERY, "hello"));
    in_tokens.pop_back();
    EXPECT_THAT(
        [&]() { peak_next_token(in_tokens); },
        ::testing::ThrowsMessage<std::runtime_error>(::testing::HasSubstr("Tokens list is empty"))
    );
}