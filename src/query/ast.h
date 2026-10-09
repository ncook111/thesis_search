#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <unordered_map>
#include "../search.h"

using FieldMap = std::unordered_map<size_t, std::vector<uint32_t>>;

class ASTNode {
  public:
    virtual ~ASTNode() = default;
    virtual const FieldMap evaluate() const = 0;
    virtual void print_type() = 0;
};

class QueryNode : public ASTNode {
  public:
    QueryNode(std::vector<std::string> f, std::string q, std::string i, uint8_t e)
            : fields(f), query(q), index_path(i), enc_type(e) {}
    
    const FieldMap evaluate() const override {
      return search_index(fields, query, index_path, enc_type);
    }

    void print_type() override {
      std::cout << "QueryNode\n";
    }

    const std::vector<std::string> get_fields() const {
      return fields;
    }

    const std::string get_query() const {
      return query;
    }

    const std::string get_index_path() const {
      return index_path;
    }

    const uint8_t get_enc_type() const {
      return enc_type;
    }

  protected:
    const std::vector<std::string> fields;
    const std::string query;
    const std::string index_path;
    const uint8_t enc_type;
};

class BinaryOpNode : public ASTNode {
  public:
    BinaryOpNode(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
          : left(std::move(l)), right(std::move(r)) {}

    const ASTNode* get_left() const {
      return left.get();
    }

    const ASTNode* get_right() const {
      return right.get();
    }
  protected:
    const std::unique_ptr<ASTNode> left;
    const std::unique_ptr<ASTNode> right;
};

class UnaryOpNode : public ASTNode {
  public:
    UnaryOpNode(std::unique_ptr<ASTNode> n, std::string i, uint8_t e)
          : node(std::move(n)), index_path(i), enc_type(e) {}
  
  const ASTNode* get_node() const {
      return node.get();
    }
  protected:
    const std::unique_ptr<ASTNode> node;
    const std::string index_path;
    const uint8_t enc_type;
};

class ComplimentNode : public UnaryOpNode {
  public:
    using UnaryOpNode::UnaryOpNode;
    const FieldMap evaluate() const override {
      return get_entries_offsets_compliment(node->evaluate(), index_path, enc_type);
    }

    void print_type() override {
      std::cout << "ComplimentNode\n";
      node->print_type();
    }
};

class UnionNode : public BinaryOpNode {
  public:
    using BinaryOpNode::BinaryOpNode;
    const FieldMap evaluate() const override {
      return get_entries_offsets_union(left->evaluate(), right->evaluate());
    }

    void print_type() override {
      std::cout << "UnionNode\n";
      left->print_type();
      right->print_type();
    }
};

class DifferenceNode : public BinaryOpNode {
  public:
    using BinaryOpNode::BinaryOpNode;
    const FieldMap evaluate() const override {
      return get_entries_offsets_difference(left->evaluate(), right->evaluate());
    }

    void print_type() override {
      std::cout << "DifferenceNode\n";
      left->print_type();
      right->print_type();
    }
};

class SymDifferenceNode : public BinaryOpNode {
  public:
    using BinaryOpNode::BinaryOpNode;
    const FieldMap evaluate() const override {
      return get_entries_offsets_sym_difference(left->evaluate(), right->evaluate());
    }

    void print_type() override {
      std::cout << "SymDifferenceNode\n";
      left->print_type();
      right->print_type();
    }
};

class IntersectionNode : public BinaryOpNode {
    public:
      using BinaryOpNode::BinaryOpNode;
      const FieldMap evaluate() const override {
        return get_entries_offsets_intersection(left->evaluate(), right->evaluate());
      }

      void print_type() override {
        std::cout << "IntersectionNode\n";
        left->print_type();
        right->print_type();
      }
};

std::unique_ptr<ASTNode> build_operand(std::vector<std::pair<uint8_t, std::string>> &tokens,
                                        const std::string &index_path, const uint8_t enc_type);
std::unique_ptr<ASTNode> build_ast(std::vector<std::pair<uint8_t, std::string>> &tokens,
                                    const std::string &index_path, const uint8_t enc_type);
std::unique_ptr<ASTNode> build_query(std::vector<std::pair<uint8_t, std::string>> &tokens, const std::string &index_path, const uint8_t enc_type);
std::unique_ptr<ASTNode> build_binary_op(std::unique_ptr<ASTNode> left, std::vector<std::pair<uint8_t, std::string>> &tokens, const std::string &index_path, const uint8_t enc_type);
std::unique_ptr<ASTNode> build_unary_op(std::vector<std::pair<uint8_t, std::string>> &tokens, const std::string &index_path, const uint8_t enc_type);
std::vector<std::string> get_fields(const std::string& s);

#endif