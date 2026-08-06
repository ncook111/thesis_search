#include "ast.h"
#include "parser.h"
#include "../helper.h"
#include <memory>
#include <regex>

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
    } else if (c == '{') {
        throw std::invalid_argument("Misplaced '{'");
    }
}

void tokenize_field(bool &in_field, const char c, std::vector<std::pair<uint8_t, std::string>> &tokens, std::string &curr_token) {
    if (c == ']') {
        in_field = false;
        check_fields(curr_token);
        tokens.push_back({QueryTokType::FIELD, curr_token});
        curr_token.clear();
    } else if (c == '[') {
       throw std::invalid_argument("Misplaced '["); 
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
            in_field = true;
            break;
        // Match closing square bracked
        case ']':
            throw std::invalid_argument("Misplaced ']'");
        // Match opening curly bracket
        case '{':
            in_rng_query = true;
            curr_token.push_back(c);
            break;
        // Match closing curly bracket
        case '}':
            throw std::invalid_argument("Misplaced '}'");
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
        throw std::invalid_argument("Query must begin with either a opening parenthesis or a field");

    while (tokens.size() >= 2) {
        check_next_expected(get_next_token(tokens), peak_next_token(tokens));
    }

    // Check last token correct
    if (tokens.back().first != QueryTokType::RPAREN &&
        tokens.back().first != QueryTokType::STDQUERY &&
        tokens.back().first != QueryTokType::RNGQUERY)
        throw std::invalid_argument("Query must begin with either a opening parenthesis or a field declaration");
    
    if (tokens.back().first == QueryTokType::RNGQUERY) {
        if (!is_range_query(tokens.back().second))
            throw std::invalid_argument("Invalid range query. Received: " + tokens.back().second);
    }
}

void check_next_expected(const std::pair<uint8_t, std::string> &curr, const std::pair<uint8_t, std::string> &next) {
    const auto &[curr_type, curr_value] = curr;
    const auto &[next_type, next_value] = next;
    switch (curr_type) {
        case (QueryTokType::LPAREN):
            if (next_type != QueryTokType::FIELD)
                throw std::invalid_argument("Expected a field declaration after opening parenthesis. Received: " + next_value);
            break;
        case (QueryTokType::RPAREN):
            if (!is_binary_op(next_type))
                throw std::invalid_argument("Expected a binary operator after closing parenthesis. Received: " + next_value);
            break;
        case (QueryTokType::FIELD):
            if (next_type != QueryTokType::EQ)
                throw std::invalid_argument("Expected an equals sign after a field declaration. Received: " + next_value);
            break;
        // Treated the same
        case (QueryTokType::RNGQUERY): [[fallthrough]];
        case (QueryTokType::STDQUERY): 
            if (!is_binary_op(next_type) && next_type != QueryTokType::RPAREN)
                throw std::invalid_argument("Expected a binary operator or right parenthesis after query declaration. Received: " + next_value);
            break;
        case (QueryTokType::EQ):
            if (next_type != QueryTokType::STDQUERY && next_type != QueryTokType::RNGQUERY)
                throw std::invalid_argument("Expected a standard or range query declaration after an equals sign. Received: " + next_value);
            break;
        case (QueryTokType::INTER): [[fallthrough]];
        case (QueryTokType::UNION): [[fallthrough]];
        case (QueryTokType::DIFF): [[fallthrough]];
        case (QueryTokType::SYMDIFF):
            if (next_type != QueryTokType::LPAREN && next_type != QueryTokType::FIELD && next_type != QueryTokType::COMPL)
                throw std::invalid_argument("Expected either an opening parenthesis, unary operator or field declaration. Received: " + next_value);
            break;
        case (QueryTokType::COMPL):
            if (next_type != QueryTokType::LPAREN && next_type != QueryTokType::FIELD)
                throw std::invalid_argument("Expected either an opening parenthesis  or field declaration. Received: " + next_value);
            break;
        default:
            throw std::invalid_argument("Invalid type. Received: " + next_value);
    }
}

const bool is_range_query(const std::string &query) {
  // Check if query in the form of a range query
  // Matches "[a,b]", where a and b are any combination of the characters
  // [0-9+\-.:]
  const std::regex pattern(R"(\{([0-9+\-.:]*),([0-9+\-.:]*)\})");
  std::smatch match;

  int res = std::regex_match(query, match, pattern);

  const std::string a = match[1].str();
  const std::string b = match[2].str();

  if (a == b)
    return false;

  return res;
}

const bool is_binary_op(const uint8_t type) {
    return (type == QueryTokType::INTER ||
        type == QueryTokType::UNION ||
        type == QueryTokType::DIFF ||
        type == QueryTokType::SYMDIFF);
}

void check_fields(const std::string& s) {
    size_t count = 0;
    std::stringstream ss(s);
    std::string item;
    bool match_all = false;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (item.empty())
            throw std::invalid_argument("Field cannot contain empty items");
        if (item == "*") match_all = true;
        count++;
    }

    if (match_all && count > 1)
        throw std::invalid_argument("Field cannot contain wild card and additional items");
}

const std::pair<uint8_t, std::string> get_next_token(std::vector<std::pair<uint8_t, std::string>> &tokens) {
    if (tokens.empty())
        throw std::runtime_error("Tokens list is empty");

    std::pair<uint8_t, std::string> back = tokens.back();
    tokens.pop_back();

    return back;
}

const std::pair<uint8_t, std::string> peak_next_token(const std::vector<std::pair<uint8_t, std::string>> &tokens) {
    if (tokens.empty())
        throw std::runtime_error("Tokens list is empty");
    
    return tokens.back();
}