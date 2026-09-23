#ifndef BUAA_COMPILER_VISITOR
#define BUAA_COMPILER_VISITOR

#include <cstddef>
#include <string>
#include <utility>

#include "frontend/ast.hpp"

namespace midend::visitor {
class Visitor {
 public:
  virtual ~Visitor() = default;
  virtual void visit(const frontend::ast::Node& node);

#define X(_, name) virtual void visit(const frontend::ast::name& node);
  NODE_TABLE
#undef X

 protected:
  static auto get_ident_name(const frontend::ast::Ident& ident) -> std::string;
  static auto extract_ident(const frontend::ast::Ident& ident)
      -> std::pair<std::string, size_t>;
  static auto extract_number_value(const frontend::ast::Number& number) -> int;
};
}  // namespace midend::visitor

#endif
