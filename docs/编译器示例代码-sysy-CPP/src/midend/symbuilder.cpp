#include "midend/symbuilder.hpp"

#include <cassert>
#include <memory>

#include "error.hpp"
#include "frontend/ast_cast.hpp"

namespace midend::visitor {
using frontend::ast::cast::cast;
using frontend::ast::cast::child_as;
using frontend::ast::cast::dyn_cast;

namespace {
auto line_of(const Node& node) -> size_t {
  if (node.get_type() == NodeType::TOKEN) {
    return cast<TokenNode>(node).get_token().get_line_no();
  }
  assert(!node.get_children().empty());
  return line_of(*node.get_children().front());
}

auto function_returns_value(const FuncType& type) -> bool {
  return child_as<TokenNode>(type, 0).get_token().get_type() ==
         TokenType::INTTK;
}
}  // namespace

// ConstDecl ::= 'const' BType ConstDef { ',' ConstDef } ';'
void SymbolBuilder::visit(const ConstDecl& node) {
  const auto& children = node.get_children();
  for (size_t i = 2; i + 1 < children.size(); i += 2) {
    children[i]->accept(*this);
  }
}

// ConstDef ::= Ident [ '[' ConstExp ']' ] '=' ConstInitVal
void SymbolBuilder::visit(const ConstDef& node) {
  const auto& children = node.get_children();
  auto name = declare(child_as<Ident>(node, 0));

  const bool is_array = children.size() == 6;
  if (is_array) {
    children[2]->accept(*this);
  }
  if (name) {
    SymbolPtr symbol = is_array
                           ? std::static_pointer_cast<Symbol>(
                                 std::make_shared<ArraySymbol>(true, false))
                           : std::static_pointer_cast<Symbol>(
                                 std::make_shared<VarSymbol>(true, false));
    symbol_table_.insert(*name, std::move(symbol));
  }
  children.back()->accept(*this);
}

// VarDecl ::= [ 'static' ] BType VarDef { ',' VarDef } ';'
void SymbolBuilder::visit(const VarDecl& node) {
  const auto& children = node.get_children();
  is_static_ = children.front()->get_type() == NodeType::TOKEN;
  const size_t first_definition = is_static_ ? 2 : 1;
  for (size_t i = first_definition; i + 1 < children.size(); i += 2) {
    children[i]->accept(*this);
  }
}

// VarDef ::= Ident [ '[' ConstExp ']' ] [ '=' InitVal ]
void SymbolBuilder::visit(const VarDef& node) {
  const auto& children = node.get_children();
  auto name = declare(child_as<Ident>(node, 0));
  const bool is_array = children.size() == 4 || children.size() == 6;

  if (is_array) {
    children[2]->accept(*this);
  }
  if (name) {
    SymbolPtr symbol =
        is_array ? std::static_pointer_cast<Symbol>(
                       std::make_shared<ArraySymbol>(false, is_static_))
                 : std::static_pointer_cast<Symbol>(
                       std::make_shared<VarSymbol>(false, is_static_));
    symbol_table_.insert(*name, std::move(symbol));
  }
  if (children.back()->get_type() == NodeType::INIT_VAL) {
    children.back()->accept(*this);
  }
}

// FuncDef ::= FuncType Ident '(' [ FuncFParams ] ')' Block
void SymbolBuilder::visit(const FuncDef& node) {
  const auto& children = node.get_children();
  current_function_returns_value_ =
      function_returns_value(child_as<FuncType>(node, 0));
  auto name = declare(child_as<Ident>(node, 1));

  auto function = std::make_shared<FuncSymbol>(current_function_returns_value_);
  if (name) {
    symbol_table_.insert(*name, function);
  }

  symbol_table_.enter_scope();
  current_parameters_.clear();
  if (children.size() == 6) {
    children[3]->accept(*this);
  }
  function->set_param_list(std::move(current_parameters_));

  const auto& block = cast<Block>(*children.back());
  block.accept(*this);
  if (current_function_returns_value_) {
    check_final_return(block);
  }
  symbol_table_.exit_scope();
}

void SymbolBuilder::visit(const MainFuncDef& node) {
  current_function_returns_value_ = true;
  symbol_table_.insert_main();
  symbol_table_.enter_scope();

  const auto& block = cast<Block>(*node.get_children().back());
  block.accept(*this);
  check_final_return(block);
  symbol_table_.exit_scope();
}

// FuncFParam ::= BType Ident [ '[' ']' ]
void SymbolBuilder::visit(const FuncFParam& node) {
  const auto& children = node.get_children();
  auto name = declare(child_as<Ident>(node, 1));
  SymbolPtr parameter = children.size() == 4
                            ? std::static_pointer_cast<Symbol>(
                                  std::make_shared<ArraySymbol>(false, false))
                            : std::static_pointer_cast<Symbol>(
                                  std::make_shared<VarSymbol>(false, false));
  current_parameters_.push_back(parameter);
  if (name) {
    symbol_table_.insert(*name, std::move(parameter));
  }
}

void SymbolBuilder::visit(const Stmt& node) {
  const auto& children = node.get_children();
  switch (node.get_stmt_type()) {
    case Stmt::StmtType::BLOCK:
      symbol_table_.enter_scope();
      children.front()->accept(*this);
      symbol_table_.exit_scope();
      return;
    case Stmt::StmtType::ASSIGN:
      children[0]->accept(*this);
      check_assignable(cast<LVal>(*children[0]));
      children[2]->accept(*this);
      return;
    case Stmt::StmtType::RETURN:
      if (children.size() == 3) {
        children[1]->accept(*this);
        if (!current_function_returns_value_) {
          report_error(ErrorType::SEM_FN_RET_TYPE, line_of(*children[0]));
        }
      }
      return;
    case Stmt::StmtType::PRINTF:
      check_printf(node);
      return;
    case Stmt::StmtType::FOR:
      for (size_t i = 0; i + 1 < children.size(); ++i) {
        children[i]->accept(*this);
      }
      ++loop_depth_;
      children.back()->accept(*this);
      --loop_depth_;
      return;
    case Stmt::StmtType::BREAK:
    case Stmt::StmtType::CONTINUE:
      if (loop_depth_ == 0) {
        report_error(ErrorType::SEM_BREAK_CONTINUE, line_of(*children[0]));
      }
      return;
    case Stmt::StmtType::EXP:
    case Stmt::StmtType::IF:
      Visitor::visit(static_cast<const Node&>(node));
      return;
  }
}

// ForStmt ::= LVal '=' Exp { ',' LVal '=' Exp }
void SymbolBuilder::visit(const ForStmt& node) {
  const auto& children = node.get_children();
  for (size_t i = 0; i < children.size(); i += 4) {
    children[i]->accept(*this);
    check_assignable(cast<LVal>(*children[i]));
    children[i + 2]->accept(*this);
  }
}

// LVal ::= Ident [ '[' Exp ']' ]
void SymbolBuilder::visit(const LVal& node) {
  lookup(child_as<Ident>(node, 0));
  if (node.get_children().size() == 4) {
    node.get_children()[2]->accept(*this);
  }
}

// UnaryExp ::= PrimaryExp | Ident '(' [ FuncRParams ] ')' | UnaryOp UnaryExp
void SymbolBuilder::visit(const UnaryExp& node) {
  const auto& children = node.get_children();
  if (children.front()->get_type() != NodeType::IDENT) {
    Visitor::visit(static_cast<const Node&>(node));
    return;
  }

  const auto ident = Visitor::extract_ident(cast<Ident>(*children[0]));
  auto symbol = lookup(ident);
  const auto* arguments =
      children.size() == 4 ? dyn_cast<FuncRParams>(children[2].get()) : nullptr;
  if (arguments) {
    arguments->accept(*this);
  }
  if (!symbol || (*symbol)->get_kind() != Symbol::SymbolKind::FUNC) {
    return;
  }
  check_call(std::static_pointer_cast<FuncSymbol>(*symbol), arguments,
             ident.second);
}

auto SymbolBuilder::declare(const Ident& ident) -> std::optional<std::string> {
  const auto [name, line] = Visitor::extract_ident(ident);
  if (symbol_table_.current_scope_contains(name)) {
    report_error(ErrorType::SEM_IDENT_REDEF, line);
    return std::nullopt;
  }
  return name;
}

auto SymbolBuilder::lookup(const Ident& ident) -> std::optional<SymbolPtr> {
  return lookup(Visitor::extract_ident(ident));
}

auto SymbolBuilder::lookup(const std::pair<std::string, size_t>& ident)
    -> std::optional<SymbolPtr> {
  auto symbol = symbol_table_.get(ident.first);
  if (!symbol) {
    report_error(ErrorType::SEM_IDENT_UNDEF, ident.second);
  }
  return symbol;
}

void SymbolBuilder::check_call(const std::shared_ptr<FuncSymbol>& function,
                               const FuncRParams* arguments, size_t line) {
  const auto& parameters = function->get_param_list();
  const size_t argument_count =
      arguments ? (arguments->get_children().size() + 1) / 2 : 0;
  if (parameters.size() != argument_count) {
    report_error(ErrorType::SEM_FN_PARAM_NUM, line);
    return;
  }

  if (!arguments) {
    return;
  }
  const auto& children = arguments->get_children();
  for (size_t i = 0; i < parameters.size(); ++i) {
    const bool expects_array =
        parameters[i]->get_kind() == Symbol::SymbolKind::ARRAY;
    const auto actual = value_category(cast<Exp>(*children[i * 2]));
    const bool matches = expects_array ? actual == ValueCategory::ARRAY
                                       : actual == ValueCategory::SCALAR;
    if (!matches && actual != ValueCategory::INVALID) {
      report_error(ErrorType::SEM_FN_PARAM_TYPE, line);
      return;
    }
  }
}

auto SymbolBuilder::value_category(const Exp& expression) const
    -> ValueCategory {
  return value_category(child_as<AddExp>(expression, 0));
}

auto SymbolBuilder::value_category(const AddExp& expression) const
    -> ValueCategory {
  return expression.get_children().size() == 1
             ? value_category(child_as<MulExp>(expression, 0))
             : ValueCategory::SCALAR;
}

auto SymbolBuilder::value_category(const MulExp& expression) const
    -> ValueCategory {
  return expression.get_children().size() == 1
             ? value_category(child_as<UnaryExp>(expression, 0))
             : ValueCategory::SCALAR;
}

auto SymbolBuilder::value_category(const UnaryExp& expression) const
    -> ValueCategory {
  const auto& children = expression.get_children();
  if (children.size() == 1) {
    return value_category(child_as<PrimaryExp>(expression, 0));
  }
  if (children.front()->get_type() == NodeType::UNARY_OP) {
    return ValueCategory::SCALAR;
  }

  const auto name = Visitor::get_ident_name(cast<Ident>(*children[0]));
  auto symbol = symbol_table_.get(name);
  if (!symbol || (*symbol)->get_kind() != Symbol::SymbolKind::FUNC) {
    return ValueCategory::INVALID;
  }
  return std::static_pointer_cast<FuncSymbol>(*symbol)->returns_value()
             ? ValueCategory::SCALAR
             : ValueCategory::VOID_VALUE;
}

auto SymbolBuilder::value_category(const PrimaryExp& expression) const
    -> ValueCategory {
  const auto& first = *expression.get_children().front();
  if (first.get_type() == NodeType::TOKEN) {
    return value_category(child_as<Exp>(expression, 1));
  }
  if (first.get_type() == NodeType::LVAL) {
    return value_category(cast<LVal>(first));
  }
  return ValueCategory::SCALAR;
}

auto SymbolBuilder::value_category(const LVal& expression) const
    -> ValueCategory {
  const auto name = Visitor::get_ident_name(child_as<Ident>(expression, 0));
  auto symbol = symbol_table_.get(name);
  if (!symbol) {
    return ValueCategory::INVALID;
  }
  if ((*symbol)->get_kind() != Symbol::SymbolKind::ARRAY ||
      expression.get_children().size() == 4) {
    return ValueCategory::SCALAR;
  }
  return (*symbol)->is_const() ? ValueCategory::CONST_ARRAY
                               : ValueCategory::ARRAY;
}

void SymbolBuilder::check_final_return(const Block& block) {
  const auto& children = block.get_children();
  const auto report_missing = [&] {
    report_error(ErrorType::SEM_FN_RET_MISS, line_of(*children.back()));
  };
  if (children.size() == 2) {
    report_missing();
    return;
  }

  const auto& last_item = children[children.size() - 2]->get_children().front();
  if (last_item->get_type() != NodeType::STMT) {
    report_missing();
    return;
  }
  const auto& statement = cast<Stmt>(*last_item);
  if (statement.get_stmt_type() != Stmt::StmtType::RETURN ||
      statement.get_children().size() != 3) {
    report_missing();
  }
}

void SymbolBuilder::check_assignable(const LVal& lval) {
  const auto [name, line] = Visitor::extract_ident(child_as<Ident>(lval, 0));
  auto symbol = symbol_table_.get(name);
  if (symbol && (*symbol)->is_const()) {
    report_error(ErrorType::SEM_CONST_ASSIGN, line);
  }
}

void SymbolBuilder::check_printf(const Stmt& statement) {
  const auto& children = statement.get_children();
  const auto& format = cast<TokenNode>(*children[2]).get_token().get_content();
  size_t placeholders = 0;
  for (size_t i = 0; i + 1 < format.size(); ++i) {
    if (format[i] == '%' && format[i + 1] == 'd') {
      ++placeholders;
      ++i;
    }
  }

  const size_t arguments = (children.size() - 5) / 2;
  if (placeholders != arguments) {
    report_error(ErrorType::SEM_PRINTF_MISMATCH, line_of(*children[0]));
  }
  for (size_t i = 0; i < arguments; ++i) {
    children[4 + i * 2]->accept(*this);
  }
}

}  // namespace midend::visitor
