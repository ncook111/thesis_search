#include "ast.h"
#include "parser.h"
#include "../helper.h"
#include "../metadata.h"
#include <ranges>
#include <memory>

std::unique_ptr<ASTNode> build_operand(std::vector<std::pair<uint8_t, std::string>> &tokens, const Metadata &metadata) {
    switch (peak_next_token(tokens).first) {
        case QueryTokType::LPAREN: {
            tokens.pop_back();
            return build_ast(tokens, metadata);
        }
        case QueryTokType::FIELD:
            return build_query(tokens, metadata);
        case QueryTokType::COMPL:
            return build_unary_op(tokens, metadata);
        default:
            throw std::invalid_argument("Expected operand. Value: " + peak_next_token(tokens).second);
    }
}

std::unique_ptr<ASTNode> build_ast(std::vector<std::pair<uint8_t, std::string>> &tokens, const Metadata &metadata) {
    std::unique_ptr<ASTNode> ast = build_operand(tokens, metadata);

    while (!tokens.empty()) {
        const uint8_t op = peak_next_token(tokens).first;
        if (peak_next_token(tokens).first == QueryTokType::RPAREN) {
            tokens.pop_back();
            return ast;
        }

        ast = build_binary_op(std::move(ast), tokens, metadata);
    }

    return ast;
}

std::unique_ptr<ASTNode> build_query(std::vector<std::pair<uint8_t, std::string>> &tokens, const Metadata &metadata) {
    std::vector<std::string> fields = get_fields(get_next_token(tokens).second);
    tokens.pop_back();
    std::string query = get_next_token(tokens).second;

    return std::make_unique<QueryNode>(fields, query, metadata);
}

std::unique_ptr<ASTNode> build_binary_op(std::unique_ptr<ASTNode> ast, std::vector<std::pair<uint8_t, std::string>> &tokens, const Metadata &metadata) {
    const auto &[op, value] = get_next_token(tokens);
    std::unique_ptr<ASTNode> rhs = build_operand(tokens, metadata);

    switch (op) {
        case QueryTokType::UNION:
            ast = std::make_unique<UnionNode>(std::move(ast), std::move(rhs));
            break;
        case QueryTokType::INTER:
            ast = std::make_unique<IntersectionNode>(std::move(ast), std::move(rhs));
            break;
        case QueryTokType::DIFF:
            ast = std::make_unique<DifferenceNode>(std::move(ast), std::move(rhs));
            break;
        case QueryTokType::SYMDIFF:
            ast = std::make_unique<SymDifferenceNode>(std::move(ast), std::move(rhs));
            break;
        default:
            throw std::invalid_argument("Unknown binary operator. Value: " + value);
    }

    return ast;
}

std::unique_ptr<ASTNode> build_unary_op(std::vector<std::pair<uint8_t, std::string>> &tokens, const Metadata &metadata) {
    std::unique_ptr<ASTNode> ast = NULL;
    const auto &[op, value] = get_next_token(tokens);
    switch (op) {
        case (QueryTokType::COMPL):
            if (peak_next_token(tokens).first == QueryTokType::LPAREN) {
                ast = std::make_unique<ComplimentNode>(std::move(build_ast(tokens, metadata)), metadata);
            } else if (peak_next_token(tokens).first == QueryTokType::FIELD) {
                ast = std::make_unique<ComplimentNode>(std::move(build_query(tokens, metadata)), metadata);
            }
            break;
        default:
            throw std::invalid_argument("Unknown unary operator. Value: " + value);
    }

    return ast;
}

std::vector<std::string> get_fields(const std::string& s) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (item.empty()) continue;
        result.push_back(item);
    }

    // If wildcard, make fields empty, easier to process going forward
    if (result.size() == 1 && result.front() == "*")
        result.pop_back();

    return result;
}