#include <gtest/gtest.h>
#include "../src/query/ast.h"
#include "../src/query/parser.h"

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
    std::string fields = "table,chair";
    std::vector<std::string> fields_vec = {"table", "chair"};
    std::string query = "apple";
    std::vector<std::pair<uint8_t, std::string>> in_tokens = {{QueryTokType::STDQUERY, query}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::GVB;
    std::unique_ptr<ASTNode> out = build_query(in_tokens, index_path, enc_type);

    ASSERT_TRUE(in_tokens.empty());
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_index_path(), index_path);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_query(), query);
}

TEST(build_query_test, test2) {
    std::string fields = "1,2";
    std::vector<std::string> fields_vec = {"1", "2"};
    std::string query = "going to the park";
    std::vector<std::pair<uint8_t, std::string>> in_tokens = {{QueryTokType::STDQUERY, query}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    std::unique_ptr<ASTNode> out = build_query(in_tokens, index_path, enc_type);

    ASSERT_TRUE(in_tokens.empty());
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_index_path(), index_path);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_query(), query);
}

TEST(build_query_test, test3) {
    std::string fields = "";
    std::vector<std::string> fields_vec = {};
    std::string query = "";
    std::vector<std::pair<uint8_t, std::string>> in_tokens = {{QueryTokType::STDQUERY, query}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    std::unique_ptr<ASTNode> out = build_query(in_tokens, index_path, enc_type);

    ASSERT_TRUE(in_tokens.empty());
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_index_path(), index_path);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_query(), query);
}

TEST(build_query_test, test4) {
    std::string fields = "*";
    std::vector<std::string> fields_vec = {}; // get_fields should return empty vec if "*" is the only field
    std::string query = "";
    std::vector<std::pair<uint8_t, std::string>> in_tokens = {{QueryTokType::STDQUERY, query}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, "*"}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    std::unique_ptr<ASTNode> out = build_query(in_tokens, index_path, enc_type);

    ASSERT_TRUE(in_tokens.empty());
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_index_path(), index_path);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<QueryNode*>(out.get())->get_query(), query);
}

/* Test build_binary_op */
TEST(build_binary_op_test, test1) {
    std::string fields = "table,chair";
    std::vector<std::string> fields_vec = {"table", "chair"};
    std::string query1 = "apple";
    std::vector<std::pair<uint8_t, std::string>> in_tokens1 = {{QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::GVB;
    std::unique_ptr<ASTNode> left = build_query(in_tokens1, index_path, enc_type);
    
    std::string query2 = "orange";
    std::vector<std::pair<uint8_t, std::string>> in_tokens2 = {{QueryTokType::STDQUERY, query2}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}, {QueryTokType::INTER, "&"}};
    
    std::unique_ptr<ASTNode> out = build_binary_op(std::move(left), in_tokens2, index_path, enc_type);
    
    ASSERT_TRUE(in_tokens1.empty());
    ASSERT_TRUE(in_tokens2.empty());
    ASSERT_EQ(typeid(*out), typeid(IntersectionNode));
    
    const ASTNode* l = static_cast<BinaryOpNode*>(out.get())->get_left();
    
    ASSERT_EQ(typeid(*l), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_query(), query1);

    const ASTNode* r = static_cast<BinaryOpNode*>(out.get())->get_right();
    
    ASSERT_EQ(typeid(*r), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_query(), query2);
}

TEST(build_binary_op_test, test2) {
    std::string fields = "table,chair,garbage";
    std::vector<std::string> fields_vec = {"table", "chair", "garbage"};
    std::string query1 = "garbage";
    std::vector<std::pair<uint8_t, std::string>> in_tokens1 = {{QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    std::unique_ptr<ASTNode> left = build_query(in_tokens1, index_path, enc_type);
    
    std::string query2 = "orange";
    std::vector<std::pair<uint8_t, std::string>> in_tokens2 = {{QueryTokType::STDQUERY, query2}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}, {QueryTokType::UNION, "|"}};
    
    std::unique_ptr<ASTNode> out = build_binary_op(std::move(left), in_tokens2, index_path, enc_type);
    
    ASSERT_TRUE(in_tokens1.empty());
    ASSERT_TRUE(in_tokens2.empty());
    ASSERT_EQ(typeid(*out), typeid(UnionNode));
    
    const ASTNode* l = static_cast<BinaryOpNode*>(out.get())->get_left();
    
    ASSERT_EQ(typeid(*l), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_query(), query1);

    const ASTNode* r = static_cast<BinaryOpNode*>(out.get())->get_right();
    
    ASSERT_EQ(typeid(*r), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_query(), query2);
}

TEST(build_binary_op_test, test3) {
    std::string fields = "table,chair,garbage";
    std::vector<std::string> fields_vec = {"table", "chair", "garbage"};
    std::string query1 = "garbage";
    std::vector<std::pair<uint8_t, std::string>> in_tokens1 = {{QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    std::unique_ptr<ASTNode> left = build_query(in_tokens1, index_path, enc_type);
    
    std::string query2 = "orange";
    std::vector<std::pair<uint8_t, std::string>> in_tokens2 = {{QueryTokType::STDQUERY, query2}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}, {QueryTokType::DIFF, "-"}};
    
    std::unique_ptr<ASTNode> out = build_binary_op(std::move(left), in_tokens2, index_path, enc_type);
    
    ASSERT_TRUE(in_tokens1.empty());
    ASSERT_TRUE(in_tokens2.empty());
    ASSERT_EQ(typeid(*out), typeid(DifferenceNode));
    
    const ASTNode* l = static_cast<BinaryOpNode*>(out.get())->get_left();
    
    ASSERT_EQ(typeid(*l), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_query(), query1);

    const ASTNode* r = static_cast<BinaryOpNode*>(out.get())->get_right();
    
    ASSERT_EQ(typeid(*r), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_query(), query2);
}

TEST(build_binary_op_test, test4) {
    std::string fields = "table,chair,garbage";
    std::vector<std::string> fields_vec = {"table", "chair", "garbage"};
    std::string query1 = "garbage";
    std::vector<std::pair<uint8_t, std::string>> in_tokens1 = {{QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    std::unique_ptr<ASTNode> left = build_query(in_tokens1, index_path, enc_type);
    
    std::string query2 = "orange";
    std::vector<std::pair<uint8_t, std::string>> in_tokens2 = {{QueryTokType::STDQUERY, query2}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}, {QueryTokType::SYMDIFF, "/"}};
    
    std::unique_ptr<ASTNode> out = build_binary_op(std::move(left), in_tokens2, index_path, enc_type);
    
    ASSERT_TRUE(in_tokens1.empty());
    ASSERT_TRUE(in_tokens2.empty());
    ASSERT_EQ(typeid(*out), typeid(SymDifferenceNode));
    
    const ASTNode* l = static_cast<BinaryOpNode*>(out.get())->get_left();
    
    ASSERT_EQ(typeid(*l), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(l)->get_query(), query1);

    const ASTNode* r = static_cast<BinaryOpNode*>(out.get())->get_right();
    
    ASSERT_EQ(typeid(*r), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(r)->get_query(), query2);
}

TEST(build_binary_op_test, test5) {
    std::string fields = "table,chair,garbage";
    std::vector<std::string> fields_vec = {"table", "chair", "garbage"};
    std::string query1 = "garbage";
    std::vector<std::pair<uint8_t, std::string>> in_tokens1 = {{QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}};
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    std::unique_ptr<ASTNode> left = build_query(in_tokens1, index_path, enc_type);
    
    std::string query2 = "orange";
    std::vector<std::pair<uint8_t, std::string>> in_tokens2 = {{QueryTokType::STDQUERY, query2}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}, {QueryTokType::SYMDIFF, "/"}, {QueryTokType::INVALID, ""}};
    
    ASSERT_THROW(std::unique_ptr<ASTNode> out = build_binary_op(std::move(left), in_tokens2, index_path, enc_type), std::invalid_argument);
}

/* Test build_unary_op */
TEST(build_unary_op_test, test1) {
    std::string fields = "table,chair,garbage";
    std::vector<std::string> fields_vec = {"table", "chair", "garbage"};
    std::string query1 = "garbage";
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    
    std::vector<std::pair<uint8_t, std::string>> in_tokens1 = {{QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}, {QueryTokType::COMPL, "!"}};
    
    std::unique_ptr<ASTNode> out = build_unary_op(in_tokens1, index_path, enc_type);
    
    ASSERT_TRUE(in_tokens1.empty());
    ASSERT_EQ(typeid(*out), typeid(ComplimentNode));
    
    const ASTNode* n = static_cast<UnaryOpNode*>(out.get())->get_node();
    
    ASSERT_EQ(typeid(*n), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(n)->get_fields(), fields_vec);
    ASSERT_EQ(static_cast<const QueryNode*>(n)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(n)->get_enc_type(), enc_type);
    ASSERT_EQ(static_cast<const QueryNode*>(n)->get_query(), query1);
}

TEST(build_unary_op_test, test2) {
    std::string fields = "table,chair,garbage";
    std::vector<std::string> fields_vec = {"table", "chair", "garbage"};
    std::string query1 = "garbage";
    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;
    
    std::vector<std::pair<uint8_t, std::string>> in_tokens1 = {{QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, ""}, {QueryTokType::FIELD, fields}, {QueryTokType::COMPL, "!"}, {QueryTokType::INVALID, ""}};
    
    ASSERT_THROW(std::unique_ptr<ASTNode> out = build_unary_op(in_tokens1, index_path, enc_type), std::invalid_argument);
}

/* Test build_ast */
TEST(build_ast_test, test1) {
    std::string fields1 = "table,chair,garbage";
    std::vector<std::string> fields_vec1 = {"table", "chair", "garbage"};
    std::string query1 = "testing testing 1 2 3";

    std::string fields2 = "table";
    std::vector<std::string> fields_vec2 = {"table"};
    std::string query2 = "hello, how are you";

    std::string fields3 = "table,chair";
    std::vector<std::string> fields_vec3 = {"table", "chair"};
    std::string query3 = "";

    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;

    std::vector<std::pair<uint8_t, std::string>> in_tokens = {{QueryTokType::STDQUERY, query3}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, fields3}, {QueryTokType::UNION, "|"}, {QueryTokType::STDQUERY, query2}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, fields2}, {QueryTokType::INTER, "&"}, {QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, fields1}, {QueryTokType::COMPL, "!"}};
    std::unique_ptr<ASTNode> ast = build_ast(in_tokens, index_path, enc_type);

    ASSERT_TRUE(in_tokens.empty());
    ASSERT_EQ(typeid(*ast), typeid(IntersectionNode));

    const ASTNode* next_left = static_cast<IntersectionNode*>(ast.get())->get_left();
    ASSERT_EQ(typeid(*next_left), typeid(ComplimentNode));

    const ASTNode* child = static_cast<const ComplimentNode*>(next_left)->get_node();
    ASSERT_EQ(typeid(*child), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(child)->get_fields(), fields_vec1);
    ASSERT_EQ(static_cast<const QueryNode*>(child)->get_query(), query1);
    ASSERT_EQ(static_cast<const QueryNode*>(child)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(child)->get_enc_type(), enc_type);

    const ASTNode* next_right = static_cast<const IntersectionNode*>(ast.get())->get_right();
    ASSERT_EQ(typeid(*next_right), typeid(UnionNode));

    next_left = static_cast<const UnionNode*>(next_right)->get_left();
    ASSERT_EQ(typeid(*next_left), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(next_left)->get_fields(), fields_vec2);
    ASSERT_EQ(static_cast<const QueryNode*>(next_left)->get_query(), query2);
    ASSERT_EQ(static_cast<const QueryNode*>(next_left)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(next_left)->get_enc_type(), enc_type);

    next_right = static_cast<const UnionNode*>(next_right)->get_right();
    ASSERT_EQ(typeid(*next_right), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(next_right)->get_fields(), fields_vec3);
    ASSERT_EQ(static_cast<const QueryNode*>(next_right)->get_query(), query3);
    ASSERT_EQ(static_cast<const QueryNode*>(next_right)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(next_right)->get_enc_type(), enc_type);
}

TEST(build_ast_test, test2) {
    std::string fields1 = "table,chair,garbage";
    std::vector<std::string> fields_vec1 = {"table", "chair", "garbage"};
    std::string query1 = "testing testing 1 2 3";

    std::string fields2 = "table";
    std::vector<std::string> fields_vec2 = {"table"};
    std::string query2 = "hello, how are you";

    std::string fields3 = "table,chair";
    std::vector<std::string> fields_vec3 = {"table", "chair"};
    std::string query3 = "";

    std::string index_path = "/test_index_path";
    uint8_t enc_type = EncType::PEF;

    std::vector<std::pair<uint8_t, std::string>> in_tokens = {{QueryTokType::STDQUERY, query3}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, fields3}, {QueryTokType::UNION, "|"}, {QueryTokType::STDQUERY, query2}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, fields2}, {QueryTokType::INTER, "&"}, {QueryTokType::RPAREN, ")"}, {QueryTokType::STDQUERY, query1}, {QueryTokType::EQ, "="}, {QueryTokType::FIELD, fields1}, {QueryTokType::LPAREN, "("}, {QueryTokType::COMPL, "!"}};
    std::unique_ptr<ASTNode> ast = build_ast(in_tokens, index_path, enc_type);

    ASSERT_TRUE(in_tokens.empty());
    ASSERT_EQ(typeid(*ast), typeid(IntersectionNode));

    const ASTNode* next_left = static_cast<IntersectionNode*>(ast.get())->get_left();
    ASSERT_EQ(typeid(*next_left), typeid(ComplimentNode));

    const ASTNode* child = static_cast<const ComplimentNode*>(next_left)->get_node();
    ASSERT_EQ(typeid(*child), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(child)->get_fields(), fields_vec1);
    ASSERT_EQ(static_cast<const QueryNode*>(child)->get_query(), query1);
    ASSERT_EQ(static_cast<const QueryNode*>(child)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(child)->get_enc_type(), enc_type);

    const ASTNode* next_right = static_cast<const IntersectionNode*>(ast.get())->get_right();
    ASSERT_EQ(typeid(*next_right), typeid(UnionNode));

    next_left = static_cast<const UnionNode*>(next_right)->get_left();
    ASSERT_EQ(typeid(*next_left), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(next_left)->get_fields(), fields_vec2);
    ASSERT_EQ(static_cast<const QueryNode*>(next_left)->get_query(), query2);
    ASSERT_EQ(static_cast<const QueryNode*>(next_left)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(next_left)->get_enc_type(), enc_type);

    next_right = static_cast<const UnionNode*>(next_right)->get_right();
    ASSERT_EQ(typeid(*next_right), typeid(QueryNode));
    ASSERT_EQ(static_cast<const QueryNode*>(next_right)->get_fields(), fields_vec3);
    ASSERT_EQ(static_cast<const QueryNode*>(next_right)->get_query(), query3);
    ASSERT_EQ(static_cast<const QueryNode*>(next_right)->get_index_path(), index_path);
    ASSERT_EQ(static_cast<const QueryNode*>(next_right)->get_enc_type(), enc_type);
}