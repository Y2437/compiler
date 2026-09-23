#include "midend/visitor.hpp"

#include <stdexcept>

#include "frontend/ast_cast.hpp"
#include "frontend/token.hpp"

namespace midend::visitor {
using frontend::ast::Node;
using frontend::ast::TokenNode;
using frontend::ast::cast::child_as;
using frontend::token::TokenType;

void Visitor::visit(const Node& node) {
  for (const auto& child : node.get_children()) {
    child->accept(*this);
  }
}

#define X(_, name)                                       \
  void Visitor::visit(const frontend::ast::name& node) { \
    visit(static_cast<const Node&>(node));               \
  }
NODE_TABLE
#undef X

auto Visitor::get_ident_name(const frontend::ast::Ident& ident) -> std::string {
  return child_as<TokenNode>(ident, 0).get_token().get_content();
}

auto Visitor::extract_ident(const frontend::ast::Ident& ident)
    -> std::pair<std::string, size_t> {
  const auto& token = child_as<TokenNode>(ident, 0).get_token();
  return {token.get_content(), token.get_line_no()};
}

auto Visitor::extract_number_value(const frontend::ast::Number& number) -> int {
  const auto& token = child_as<TokenNode>(number, 0).get_token();
  if (token.get_type() != TokenType::INTCON) {
    throw std::runtime_error("invalid number token type");
  }
  return std::stoi(token.get_content());
}

}  // namespace midend::visitor
