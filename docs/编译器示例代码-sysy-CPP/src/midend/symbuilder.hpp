#ifndef BUAA_COMPILER_SYMBUILDER
#define BUAA_COMPILER_SYMBUILDER

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "frontend/ast.hpp"
#include "midend/symtable.hpp"
#include "midend/visitor.hpp"

namespace midend::visitor {
using namespace frontend::ast;
using namespace midend::symbol;

class SymbolBuilder : public Visitor {
 public:
  void build(const Node& root) { root.accept(*this); }

  auto& get_symbol_table() const& { return symbol_table_; }
  auto get_symbol_table() && { return std::move(symbol_table_); }

 private:
  enum class ValueCategory { SCALAR, ARRAY, CONST_ARRAY, VOID_VALUE, INVALID };

  using Visitor::visit;
  void visit(const ConstDecl& node) override;
  void visit(const ConstDef& node) override;
  void visit(const VarDecl& node) override;
  void visit(const VarDef& node) override;
  void visit(const FuncDef& node) override;
  void visit(const MainFuncDef& node) override;
  void visit(const FuncFParam& node) override;
  void visit(const Stmt& node) override;
  void visit(const ForStmt& node) override;
  void visit(const LVal& node) override;
  void visit(const UnaryExp& node) override;

  auto declare(const Ident& ident) -> std::optional<std::string>;
  auto lookup(const Ident& ident) -> std::optional<SymbolPtr>;
  auto lookup(const std::pair<std::string, size_t>& ident)
      -> std::optional<SymbolPtr>;

  void check_call(const std::shared_ptr<FuncSymbol>& function,
                  const FuncRParams* arguments, size_t line);
  auto value_category(const Exp& expression) const -> ValueCategory;
  auto value_category(const AddExp& expression) const -> ValueCategory;
  auto value_category(const MulExp& expression) const -> ValueCategory;
  auto value_category(const UnaryExp& expression) const -> ValueCategory;
  auto value_category(const PrimaryExp& expression) const -> ValueCategory;
  auto value_category(const LVal& expression) const -> ValueCategory;

  void check_final_return(const Block& block);
  void check_assignable(const LVal& lval);
  void check_printf(const Stmt& statement);

  bool is_static_ = false;
  bool current_function_returns_value_ = false;
  size_t loop_depth_ = 0;
  std::vector<SymbolPtr> current_parameters_;
  SymbolTable symbol_table_;
};
}  // namespace midend::visitor

#endif
