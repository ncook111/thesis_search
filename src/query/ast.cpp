#include "ast.h"
#include "parser.h"
#include <ranges>
#include <memory>

std::unique_ptr<ASTNode> build_ast(std::vector<std::pair<uint8_t, std::string>> &tokens, const std::string &index_path, const uint8_t enc_type) {
    std::unique_ptr<ASTNode> ast = NULL;
    while (!tokens.empty()) {
        const uint8_t next_type = peak_next_token(tokens).first;
        switch (next_type) {
            case (QueryTokType::LPAREN):
                tokens.pop_back();
                ast = build_ast(tokens, index_path, enc_type);
                break;
            case (QueryTokType::RPAREN):
                return ast;
            case (QueryTokType::FIELD):
                ast = build_query(tokens, index_path, enc_type);
                break;
            case (QueryTokType::COMPL):
                ast = build_unary_op(tokens, index_path, enc_type);
                break;
            case (QueryTokType::UNION): [[fallthrough]];
            case (QueryTokType::INTER): [[fallthrough]];
            case (QueryTokType::DIFF): [[fallthrough]];
            case (QueryTokType::SYMDIFF):
                ast = build_binary_op(std::move(ast), tokens, index_path, enc_type);
                break;
            default:
                throw std::invalid_argument("BA: Unknown type. Value: " + peak_next_token(tokens).second);
        }
    }

    return ast;
}

std::unique_ptr<ASTNode> build_query(std::vector<std::pair<uint8_t, std::string>> &tokens, const std::string &index_path, const uint8_t enc_type) {
    std::vector<std::string> fields = get_fields(get_next_token(tokens).second);
    tokens.pop_back();
    std::string query = get_next_token(tokens).second;

    return std::make_unique<QueryNode>(fields, query, index_path, enc_type);
}

std::unique_ptr<ASTNode> build_binary_op(std::unique_ptr<ASTNode> left, std::vector<std::pair<uint8_t, std::string>> &tokens, const std::string &index_path, const uint8_t enc_type) {
    std::unique_ptr<ASTNode> right = NULL;
    while (!tokens.empty()) {
        const auto &[op, value] = get_next_token(tokens);
        switch (op) {
            case QueryTokType::UNION:
                return std::make_unique<UnionNode>(std::move(left), std::move(build_ast(tokens, index_path, enc_type)));
            case QueryTokType::INTER:
                return std::make_unique<IntersectionNode>(std::move(left), std::move(build_ast(tokens, index_path, enc_type)));
            case QueryTokType::DIFF:
                return std::make_unique<DifferenceNode>(std::move(left), std::move(build_ast(tokens, index_path, enc_type)));
            case QueryTokType::SYMDIFF:
                return std::make_unique<SymDifferenceNode>(std::move(left), std::move(build_ast(tokens, index_path, enc_type)));
            default:
                throw std::invalid_argument("Unknown binary operator. Value: " + value);
        }
    }

    return right;
}

std::unique_ptr<ASTNode> build_unary_op(std::vector<std::pair<uint8_t, std::string>> &tokens, const std::string &index_path, const uint8_t enc_type) {
    std::unique_ptr<ASTNode> ast = NULL;
    while (!tokens.empty()) {
        const auto &[op, value] = get_next_token(tokens);
        switch (op) {
            case (QueryTokType::COMPL):
                return std::make_unique<ComplimentNode>(std::move(build_ast(tokens, index_path, enc_type)), index_path, enc_type);
                break;
            default:
                throw std::invalid_argument("Unknown unary operator. Value: " + value);
        }
    }

    return ast;
}

std::vector<std::string> get_fields(const std::string& s) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        result.push_back(item);
    }

    return result;
}