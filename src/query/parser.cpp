#include "ast.h"
#include "parser.h"
#include <memory>

using FieldMap = std::unordered_map<size_t, std::__1::vector<uint64_t>>;

const FieldMap parse_queries(const std::string &queries, const std::string &index_path, const uint8_t enc_type) {
    std::vector<std::pair<uint8_t, std::string>> tokens = tokenize(queries);
    for (auto &[_, value] : tokens) {
        std::cout << value << std::endl;
    }
    std::unique_ptr<ASTNode> ast = build_ast(tokens, index_path, enc_type);
    ast->print_type();
    return ast->evaluate();
}

const std::vector<std::pair<uint8_t, std::string>> tokenize(const std::string &queries) {
    std::vector<std::pair<uint8_t, std::string>> tokens;
    std::string curr_token;

    bool in_field = false;
    bool in_std_query = false;
    bool in_rng_query = false;
    for (const char c : queries) {
        // Certain reserved characters are allowed in standard queries
        if (in_std_query) {
            tokenize_std_query(in_std_query, c, tokens, curr_token);
            continue;
        }


        if (in_rng_query) {
            tokenize_rng_query(in_rng_query, c, tokens, curr_token);
            continue;
        }

        // Certain reserved characters are allowed in fields
        if (in_field) {
            tokenize_field(in_field, c, tokens, curr_token);
            continue;
        }

        tokenize_else(in_std_query, in_rng_query, in_field, c, tokens, curr_token);

    }

    std::reverse(tokens.begin(), tokens.end());
    validate_tokens(tokens);

    return tokens;
}

void tokenize_std_query(bool &in_std_query, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token) {
    if (c == '"') {
        in_std_query = false;
        tokens.push_back({QueryTokType::STDQUERY, curr_token});
        curr_token.clear();
    } else {
        curr_token.push_back(c);
    }
}

void tokenize_rng_query(bool &in_rng_query, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token) {
    curr_token.push_back(c);
    
    if (c == '}') {
        in_rng_query = false;
        tokens.push_back({QueryTokType::RNGQUERY, curr_token});
        curr_token.clear();
    }
}

void tokenize_field(bool &in_field, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token) {
    if (c == ']') {
        in_field = false;
        tokens.push_back({QueryTokType::FIELD, curr_token});
        curr_token.clear();
    } else {
        curr_token.push_back(c);
    }
}

void tokenize_else(bool &in_std_query, bool &in_rng_query, bool &in_field, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token) {
    switch (c) {
        // Match opening parenthesis 
        case '(':
            tokens.push_back({QueryTokType::LPAREN, "("});
            break;
        // Match closing parenthesis
        case ')':
            tokens.push_back({QueryTokType::RPAREN, ")"});
            break;
        // Match opening square bracked
        case '[':
            if (in_field) throw std::runtime_error("Misplaced '['");
            in_field = true;
            break;
        // Match closing square bracked
        case ']':
            throw std::runtime_error("Misplaced ']'");
        // Match opening curly bracket
        case '{':
            if (in_rng_query) throw std::runtime_error("Misplaced '{'");
            in_rng_query = true;
            curr_token.push_back(c);
            break;
        // Match closing curly bracket
        case '}':
            throw std::runtime_error("Misplaced '}'");
            break;
        // Match equals
        case '=':
            tokens.push_back({QueryTokType::EQ, "="});
            break;
        // Match intersection operator
        case '&':
            tokens.push_back({QueryTokType::INTER, "&"});
            break;
        // Match union operator
        case '|':
            tokens.push_back({QueryTokType::UNION, "|"});
            break;
        // Match difference operator
        case '-':
            tokens.push_back({QueryTokType::DIFF, "-"});
            break;
        // Match symetrical difference operator
        case '/':
            tokens.push_back({QueryTokType::SYMDIFF, "/"});
            break;
        // Match compliment operator
        case '!':
            tokens.push_back({QueryTokType::COMPL, "!"});
            break;
        // Match double quotation mark
        case '"':
            in_std_query = true;
            break;
    }
}

void validate_tokens(std::vector<std::pair<uint8_t, std::string>> tokens) {
    if (tokens.empty()) 
        throw std::runtime_error("Tokens list empty");

    // Check first token correct
    if (tokens.back().first != QueryTokType::LPAREN &&
        tokens.back().first != QueryTokType::FIELD &&
        tokens.back().first != QueryTokType::COMPL)
        throw std::runtime_error("Query must begin with either a opening parenthesis or a field");

    while (tokens.size() >= 2) {
        check_next_expected(get_next_token(tokens), peak_next_token(tokens));
    }

    // Check last token correct
    if (tokens.back().first != QueryTokType::RPAREN &&
        tokens.back().first != QueryTokType::STDQUERY &&
        tokens.back().first != QueryTokType::RNGQUERY)
        throw std::runtime_error("Query must begin with either a opening parenthesis or a field declaration");
}

void check_next_expected(const std::pair<uint8_t, std::string> &curr, const std::pair<uint8_t, std::string> &next) {
    const auto &[curr_type, curr_value] = curr;
    const auto &[next_type, next_value] = next;
    switch (curr_type) {
        case (QueryTokType::LPAREN):
            if (next_type != QueryTokType::FIELD)
                throw std::runtime_error("Expected field after opening parenthesis. Received: " + next_value);
            break;
        case (QueryTokType::RPAREN):
            if (!is_binary_op(next_type))
                throw std::runtime_error("Expected field after opening parenthesis. Received: " + next_value);
            break;
        case (QueryTokType::FIELD):
            if (next_type != QueryTokType::EQ)
                throw std::runtime_error("Expected field after opening parenthesis. Received: " + next_value);
            break;
        // Treated the same
        case (QueryTokType::STDQUERY): [[fallthrough]];
        case (QueryTokType::RNGQUERY):
            if (!is_binary_op(next_type) && next_type != QueryTokType::RPAREN)
                throw std::runtime_error("Expected a binary operator or right parenthesis. Received: " + next_value);
            break;
        case (QueryTokType::EQ):
            if (next_type != QueryTokType::STDQUERY && next_type != QueryTokType::RNGQUERY)
                throw std::runtime_error("Expected a standard or range query. Received: " + next_value);
            break;
        case (QueryTokType::INTER): [[fallthrough]];
        case (QueryTokType::UNION): [[fallthrough]];
        case (QueryTokType::DIFF): [[fallthrough]];
        case (QueryTokType::SYMDIFF):
            if (next_type != QueryTokType::LPAREN && next_type != QueryTokType::FIELD && next_type != QueryTokType::COMPL)
                throw std::runtime_error("Expected either an opening parenthesis, unary operator or field declaration. Received: " + next_value);
            break;
        case (QueryTokType::COMPL):
            if (next_type != QueryTokType::LPAREN && next_type != QueryTokType::FIELD)
                throw std::runtime_error("Expected either an opening parenthesis  or field declaration. Received: " + next_value);
            break;
        default:
            throw std::runtime_error("Invalid type. Received: " + next_value);
    }
}

bool is_binary_op(const uint8_t type) {
    return (type == QueryTokType::INTER ||
        type == QueryTokType::UNION ||
        type == QueryTokType::DIFF ||
        type == QueryTokType::SYMDIFF);
}

const std::pair<uint8_t, std::string> get_next_token(std::vector<std::pair<uint8_t, std::string>> &tokens) {
    std::pair<uint8_t, std::string> back = tokens.back();
    tokens.pop_back();

    return back;
}

const std::pair<uint8_t, std::string> peak_next_token(const std::vector<std::pair<uint8_t, std::string>> &tokens) {
    return tokens.back();
}