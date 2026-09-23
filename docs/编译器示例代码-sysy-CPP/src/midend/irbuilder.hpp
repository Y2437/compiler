#ifndef BUAA_COMPILER_IRBUILDER
#define BUAA_COMPILER_IRBUILDER

#include <list>
#include <memory>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "frontend/ast.hpp"
#include "midend/llvm/module.hpp"
#include "midend/llvm/value.hpp"
#include "midend/symtable.hpp"
#include "midend/visitor.hpp"

namespace midend::visitor {
using namespace frontend::ast;
using namespace midend::symbol;
using namespace midend::llvm;

class IrBuilder : public Visitor {
 public:
  using ValueInsts = std::pair<std::shared_ptr<Value>,
                               std::list<std::shared_ptr<Instruction>>>;

  explicit IrBuilder(SymbolTable symbol_table);

  void build(const Node& root) { root.accept(*this); }

  auto get_module() const -> const Module& { return module; }
  auto get_module() -> Module& { return module; }

  static auto gen_global_var_name() -> std::string;
  static auto gen_block_name() -> std::string;
  static auto gen_local_var_name() -> std::string;

 private:
  using Visitor::visit;

  void visit(const ConstDef& const_def) override;
  void visit(const VarDef& var_def) override;
  void visit(const FuncDef& func_def) override;
  void visit(const MainFuncDef& main_func_def) override;
  void visit(const Stmt& stmt) override;
  void visit(const LOrExp& lor_exp) override;
  void visit(const LAndExp& land_exp) override;
  void visit(const ForStmt& for_stmt) override;

  // for Stmt with multiple kinds
  void visit_assign_stmt(const Stmt& stmt);
  void visit_exp_stmt(const Stmt& stmt);
  void visit_block_stmt(const Stmt& stmt);
  void visit_if_stmt(const Stmt& stmt);
  void visit_for_stmt(const Stmt& stmt);
  void visit_break_stmt(const Stmt& stmt);
  void visit_continue_stmt(const Stmt& stmt);
  void visit_return_stmt(const Stmt& stmt);
  void visit_printf_stmt(const Stmt& stmt);

  // for Exp
  auto visit_exp(const Node& node) -> ValueInsts;
  auto visit_exp(const Exp& exp) -> ValueInsts;
  auto visit_exp(const LVal& lval) -> ValueInsts;
  auto visit_exp(const PrimaryExp& primary_exp) -> ValueInsts;
  auto visit_exp(const UnaryExp& unary_exp) -> ValueInsts;
  auto visit_exp(const MulExp& mul_exp) -> ValueInsts;
  auto visit_exp(const AddExp& add_exp) -> ValueInsts;
  auto visit_exp(const RelExp& rel_exp) -> ValueInsts;
  auto visit_exp(const EqExp& eq_exp) -> ValueInsts;

  // evaluate constant
  auto eval_const_init(const ConstInitVal& const_init_val) const
      -> std::vector<std::shared_ptr<Constant>>;
  auto eval_init(const InitVal& init_val) const
      -> std::vector<std::shared_ptr<Constant>>;
  auto eval_exp(const ConstExp& const_exp) const -> std::shared_ptr<Constant>;
  auto eval_exp(const Exp& exp) const -> std::shared_ptr<Constant>;
  auto eval_exp(const AddExp& add_exp) const -> std::shared_ptr<Constant>;
  auto eval_exp(const MulExp& mul_exp) const -> std::shared_ptr<Constant>;
  auto eval_exp(const UnaryExp& unary_exp) const -> std::shared_ptr<Constant>;
  auto eval_exp(const PrimaryExp& primary_exp) const
      -> std::shared_ptr<Constant>;
  auto eval_exp(const LVal& lval) const -> std::shared_ptr<Constant>;
  auto eval_exp(const Number& number) const -> std::shared_ptr<Constant>;

  // helper methods
  auto cal_binary_exp(const std::shared_ptr<Constant>& lhs,
                      const std::shared_ptr<Constant>& rhs,
                      frontend::token::TokenType op) const
      -> std::shared_ptr<Constant>;

  auto get_ir_type(const SymbolPtr& symbol) -> std::shared_ptr<Type>;

  auto make_type_conversion(const std::shared_ptr<Value>& val,
                            const std::shared_ptr<Type>& target_type)
      -> ValueInsts;

  auto make_binary_type_conversion(const std::shared_ptr<Value>& left,
                                   const std::shared_ptr<Value>& right)
      -> std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>,
                    std::list<std::shared_ptr<Instruction>>>;

  auto add_global_var(std::shared_ptr<Type> type,
                      const std::shared_ptr<Constant>& init_value)
      -> std::shared_ptr<GlobalVariable>;

  auto add_func(const std::string& name, const std::shared_ptr<Type>& func_type)
      -> std::shared_ptr<Function>;

  auto add_basic_block() -> std::shared_ptr<BasicBlock>;

  auto create_entry_alloca(const std::shared_ptr<Type>& content_type)
      -> std::shared_ptr<Value>;

  auto get_or_create_string_constant(const std::string& str)
      -> std::shared_ptr<GlobalVariable>;

  static auto get_func_name(const std::string& base_name) -> std::string {
    return "@" + base_name;
  }

  static auto get_arg_name(const std::string& base_name) -> std::string {
    return "%" + base_name;
  }

 private:
  SymbolTable symbol_table;
  Module module;
  std::shared_ptr<Function> current_function;
  std::shared_ptr<BasicBlock> current_basic_block;
  std::list<std::shared_ptr<BasicBlock>> true_basic_blocks;
  std::list<std::shared_ptr<BasicBlock>> false_basic_blocks;
  std::list<std::shared_ptr<BasicBlock>> loop_entry_blocks;
  std::list<std::shared_ptr<BasicBlock>> loop_exit_blocks;
  // Whether an LVal should produce its address instead of loading its value.
  std::stack<bool> is_left;
  bool in_array_arg = false;
  std::unordered_map<std::string, std::shared_ptr<GlobalVariable>>
      string_constants_cache;
};
}  // namespace midend::visitor

#endif  // BUAA_COMPILER_IRBUILDER
