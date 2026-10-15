#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "ast.h"

using FieldMap = std::unordered_map<size_t, std::vector<uint64_t>>;

namespace QueryTokType {
    enum Value : uint8_t { 
        // Used during both parsing and while building ast
        LPAREN = 0, 
        RPAREN = 1, 
        EQ = 2,
        INTER = 3,
        UNION = 4,
        DIFF = 5,
        SYMDIFF = 6,
        COMPL = 7,
        STDQUERY = 8,
        RNGQUERY = 9,
        FIELD = 10,

        // Only used during parsing
        DQUOT = 11,
        LBRACK = 12, 
        RBRACK = 13,
        LCURL = 14,
        RCURL = 15,
        INVALID = 16,
    };
}

const FieldMap parse_queries(const std::string &queries, const Metadata &metadata);
const std::vector<std::pair<uint8_t, std::string>> tokenize(const std::string &queries);
void tokenize_std_query(bool &in_std_query, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token);
void tokenize_rng_query(bool &in_rng_query, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token);
void tokenize_field(bool &in_field, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token);
void tokenize_else(bool &in_std_query, bool &in_rng_query, bool &in_field, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token);
void validate_tokens(std::vector<std::pair<uint8_t, std::string>> tokens);
void check_next_expected(const std::pair<uint8_t, std::string> &curr, const std::pair<uint8_t, std::string> &next);
const bool is_range_query(const std::string &query);
const bool is_binary_op(const uint8_t type);
const std::pair<uint8_t, std::string> get_next_token(std::vector<std::pair<uint8_t, std::string>> &tokens);
const std::pair<uint8_t, std::string> peak_next_token(const std::vector<std::pair<uint8_t, std::string>> &tokens);
void check_fields(const std::string& s);

#endif