#include "midend/irbuilder.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "error.hpp"
#include "frontend/ast.hpp"
#include "frontend/ast_cast.hpp"
#include "frontend/token.hpp"
#include "midend/llvm/instruction.hpp"
#include "midend/llvm/value.hpp"

namespace midend::visitor {
using frontend::ast::cast::cast;
using frontend::ast::cast::child_as;
using frontend::token::TokenType;

IrBuilder::IrBuilder(SymbolTable symbol_table)
    : symbol_table(std::move(symbol_table)) {
  this->symbol_table.prepare_replay();
  auto bind_builtin = [this](const std::string& name,
                             const std::shared_ptr<Function>& function) {
    auto symbol = this->symbol_table.get(name);
    ENSURE(symbol.has_value(), "Missing built-in function: " + name);
    (*symbol)->set_ir_value(function);
  };

  bind_builtin("getint", Function::getint);
}

auto IrBuilder::gen_global_var_name() -> std::string {
  static int global_var_counter = 0;
  return "@global_var_" + std::to_string(global_var_counter++);
}

auto IrBuilder::gen_block_name() -> std::string {
  static int block_counter = 0;
  return "block_" + std::to_string(block_counter++);
}

auto IrBuilder::gen_local_var_name() -> std::string {
  static int local_var_counter = 0;
  return "%local_var_" + std::to_string(local_var_counter++);
}

// ConstDef ::= Ident [ '[' ConstExp ']' ] '=' ConstInitVal
void IrBuilder::visit(const ConstDef& const_def) {
  auto symbol =
      symbol_table.get(Visitor::get_ident_name(child_as<Ident>(const_def, 0)))
          .value();

  const auto& children = const_def.get_children();
  if (symbol->get_kind() == Symbol::SymbolKind::VAR) {
    auto init_constant =
        eval_const_init(cast<ConstInitVal>(*children.back())).front();
    symbol->set_ir_value(init_constant);
    return;
  }

  auto array_symbol = std::static_pointer_cast<ArraySymbol>(symbol);
  int array_size = std::static_pointer_cast<ConstantInt>(
                       eval_exp(cast<ConstExp>(*children[2])))
                       ->get_val();

  std::vector<std::shared_ptr<Constant>> init_values =
      eval_const_init(cast<ConstInitVal>(*children.back()));
  ENSURE(init_values.size() <= static_cast<size_t>(array_size),
         "Initializer size exceeds array size");

  auto array_type = ArrayType::get(IntegerType::get(32), array_size);
  auto constant_array =
      std::make_shared<ConstantArray>(array_type, std::move(init_values));

  if (symbol_table.is_global()) {
    auto gv = add_global_var(PointerType::get(array_type), constant_array);
    array_symbol->set_ir_value(gv);
    return;
  }

  auto alloca_inst = create_entry_alloca(array_type);
  array_symbol->set_ir_value(alloca_inst);

  auto values = constant_array->get_values();
  std::vector<std::shared_ptr<Value>> indexes = {ConstantInt::create(0)};
  for (int i = 0; i < array_size; ++i) {
    std::shared_ptr<Value> val_to_store;
    if (i < static_cast<int>(values.size())) {
      val_to_store = values[i];
    } else {
      val_to_store = ConstantInt::create(0);
    }

    indexes.push_back(ConstantInt::create(i));
    auto gep = Getelementptr::create(current_basic_block, alloca_inst, indexes,
                                     gen_local_var_name());
    auto store = Store::create(current_basic_block, val_to_store, gep);
    current_basic_block->add_instructions({gep, store});
    indexes.pop_back();
  }
}

// VarDef ::= Ident [ '[' ConstExp ']' ] [ '=' InitVal ]
void IrBuilder::visit(const VarDef& var_def) {
  auto symbol =
      symbol_table.get(Visitor::get_ident_name(child_as<Ident>(var_def, 0)))
          .value();
  const auto& children = var_def.get_children();

  if (symbol->get_kind() == Symbol::SymbolKind::VAR) {
    // global or static var can be evaluated at compile time
    if (symbol_table.is_global() || symbol->is_static()) {
      std::shared_ptr<Constant> global_init =
          children.size() == 3
              ? eval_init(cast<InitVal>(*children.back())).front()
              : get_zero(get_ir_type(symbol));
      auto gv =
          add_global_var(PointerType::get(get_ir_type(symbol)), global_init);
      symbol->set_ir_value(gv);
      return;
    }

    auto alloca_inst = create_entry_alloca(get_ir_type(symbol));
    symbol->set_ir_value(alloca_inst);

    if (children.size() == 3) {
      const auto& init_exp = child_as<Exp>(*children.back(), 0);
      auto [value, insts] = visit_exp(init_exp);
      current_basic_block->add_instructions(std::move(insts));
      auto store = Store::create(current_basic_block, value, alloca_inst);
      current_basic_block->add_instructions({store});
    }

    return;
  }

  // array variable
  auto array_symbol = std::static_pointer_cast<ArraySymbol>(symbol);
  int array_size = std::static_pointer_cast<ConstantInt>(
                       eval_exp(cast<ConstExp>(*children[2])))
                       ->get_val();
  auto array_type = ArrayType::get(IntegerType::get(32), array_size);

  if (symbol_table.is_global() || symbol->is_static()) {
    std::vector<std::shared_ptr<Constant>> init_values;
    if (children.size() == 6) {
      init_values = eval_init(cast<InitVal>(*children.back()));
    }

    auto gv = add_global_var(
        PointerType::get(array_type),
        std::make_shared<ConstantArray>(array_type, std::move(init_values),
                                        array_symbol->get_name()));
    symbol->set_ir_value(gv);
    return;
  }

  auto alloca_inst = create_entry_alloca(array_type);
  symbol->set_ir_value(alloca_inst);

  if (children.size() != 6) {  // no init value
    return;
  }

  std::vector<std::shared_ptr<Value>> init_values;
  const auto& init_val = cast<InitVal>(*children.back());
  const auto& val_children = init_val.get_children();

  for (const auto& init_exp : val_children) {
    if (init_exp->get_type() == NodeType::TOKEN) {
      continue;
    }
    auto [value, insts] = visit_exp(*init_exp);
    current_basic_block->add_instructions(std::move(insts));
    init_values.push_back(value);
  }

  // Store values for index 0 to array_size - 1
  std::vector<std::shared_ptr<Value>> indexes = {ConstantInt::create(0)};
  for (int i = 0; i < array_size; ++i) {
    std::shared_ptr<Value> val_to_store;
    if (i < static_cast<int>(init_values.size())) {
      val_to_store = init_values[i];
    } else {
      val_to_store = ConstantInt::create(0);
    }

    indexes.push_back(ConstantInt::create(i));
    auto gep = Getelementptr::create(current_basic_block, alloca_inst, indexes,
                                     gen_local_var_name());
    auto store = Store::create(current_basic_block, val_to_store, gep);
    current_basic_block->add_instructions({gep, store});
    indexes.pop_back();
  }
}

void IrBuilder::visit(const FuncDef& func_def) {
  auto symbol_opt =
      symbol_table.get(Visitor::get_ident_name(child_as<Ident>(func_def, 1)));
  auto symbol = std::static_pointer_cast<FuncSymbol>(*symbol_opt);
  auto func = add_func(symbol->get_name(), get_ir_type(symbol));
  symbol->set_ir_value(func);

  symbol_table.replay_enter_scope();

  std::vector<std::shared_ptr<Argument>> args;
  args.reserve(symbol->get_param_list().size());
  for (const auto& param_symbol : symbol->get_param_list()) {
    auto arg =
        std::make_shared<Argument>(get_ir_type(param_symbol), current_function,
                                   get_arg_name(param_symbol->get_name()));
    args.push_back(arg);

    auto alloca_inst = create_entry_alloca(arg->get_type());
    auto store = Store::create(current_basic_block, arg, alloca_inst);
    current_basic_block->add_instructions({store});
    param_symbol->set_ir_value(alloca_inst);
  }
  current_function->set_arguments(std::move(args));

  // visit function body
  func_def.get_children().back()->accept(*this);

  auto terminator = current_basic_block->get_terminator();
  if (terminator == nullptr ||
      terminator->get_instruction_type() != Instruction::InstructionType::RET) {
    if (!symbol->returns_value()) {
      // void function without return statement, add it explicitly
      current_basic_block->add_instructions({Ret::create(current_basic_block)});
    }
  }

  symbol_table.replay_exit_scope();
}

void IrBuilder::visit(const MainFuncDef& main_func_def) {
  auto symbol = std::static_pointer_cast<FuncSymbol>(*symbol_table.get("main"));
  auto func = add_func(symbol->get_name(), get_ir_type(symbol));

  symbol_table.replay_enter_scope();
  main_func_def.get_children().back()->accept(*this);
  symbol_table.replay_exit_scope();
}

void IrBuilder::visit(const Stmt& stmt) {
  switch (stmt.get_stmt_type()) {
    case Stmt::StmtType::ASSIGN:
      visit_assign_stmt(stmt);
      return;
    case Stmt::StmtType::EXP:
      visit_exp_stmt(stmt);
      return;
    case Stmt::StmtType::BLOCK:
      visit_block_stmt(stmt);
      return;
    case Stmt::StmtType::IF:
      visit_if_stmt(stmt);
      return;
    case Stmt::StmtType::FOR:
      visit_for_stmt(stmt);
      return;
    case Stmt::StmtType::BREAK:
      visit_break_stmt(stmt);
      return;
    case Stmt::StmtType::CONTINUE:
      visit_continue_stmt(stmt);
      return;
    case Stmt::StmtType::RETURN:
      visit_return_stmt(stmt);
      return;
    case Stmt::StmtType::PRINTF:
      visit_printf_stmt(stmt);
      return;
    default:
      throw std::runtime_error("unsupported statement type");
  }
}

// LOrExp ::= LAndExp { '||' LAndExp }
void IrBuilder::visit(const LOrExp& lor_exp) {
  const auto& children = lor_exp.get_children();
  for (size_t i = 0; i < children.size(); i += 2) {
    auto next_block = (i + 2 >= children.size()) ? false_basic_blocks.back()
                                                 : add_basic_block();

    true_basic_blocks.push_back(true_basic_blocks.back());
    false_basic_blocks.push_back(next_block);
    children[i]->accept(*this);
    true_basic_blocks.pop_back();
    false_basic_blocks.pop_back();

    current_basic_block = next_block;
  }
}

// LAndExp ::= EqExp { '&&' EqExp }
void IrBuilder::visit(const LAndExp& land_exp) {
  const auto& children = land_exp.get_children();
  for (size_t i = 0; i < children.size(); i += 2) {
    auto next_block = (i + 2 >= children.size()) ? true_basic_blocks.back()
                                                 : add_basic_block();

    auto [cond_value, insts] = visit_exp(*children[i]);
    current_basic_block->add_instructions(std::move(insts));

    auto [as_i1, conv_insts] =
        make_type_conversion(cond_value, IntegerType::get(1));
    current_basic_block->add_instructions(std::move(conv_insts));

    current_basic_block->add_instructions({Br::create(
        current_basic_block, as_i1, next_block, false_basic_blocks.back())});
    current_basic_block = next_block;
  }
}

// ForStmt ::= LVal '=' Exp {',' LVal '=' Exp}
void IrBuilder::visit(const ForStmt& for_stmt) {
  const auto& children = for_stmt.get_children();
  for (size_t i = 0; i < children.size(); i += 4) {
    is_left.push(true);
    auto [lval_ptr, lval_insts] = visit_exp(*children[i]);
    is_left.pop();

    auto [rhs, rhs_insts] = visit_exp(*children[i + 2]);
    current_basic_block->add_instructions(std::move(lval_insts));
    current_basic_block->add_instructions(std::move(rhs_insts));

    auto store = Store::create(current_basic_block, rhs, lval_ptr);
    current_basic_block->add_instructions({store});
  }
}
}  // namespace midend::visitor

namespace midend::visitor {  // stmt implementations
void IrBuilder::visit_assign_stmt(const Stmt& stmt) {
  const auto& children = stmt.get_children();

  is_left.push(true);
  auto [lval_ptr, lhs_insts] = visit_exp(*children[0]);
  is_left.pop();

  auto [rhs_value, rhs_insts] = visit_exp(*children[2]);

  current_basic_block->add_instructions(std::move(lhs_insts));
  current_basic_block->add_instructions(std::move(rhs_insts));
  current_basic_block->add_instructions(
      {Store::create(current_basic_block, rhs_value, lval_ptr)});
}

void IrBuilder::visit_exp_stmt(const Stmt& stmt) {
  const auto& children = stmt.get_children();
  if (children.size() > 1) {
    auto [_, insts] = visit_exp(*children.front());
    current_basic_block->add_instructions(std::move(insts));
  }
}

void IrBuilder::visit_block_stmt(const Stmt& stmt) {
  symbol_table.replay_enter_scope();
  stmt.get_children().front()->accept(*this);
  symbol_table.replay_exit_scope();
}

// Stmt ::= 'if' '(' Cond ')' Stmt ['else' Stmt]
void IrBuilder::visit_if_stmt(const Stmt& stmt) {
  const auto& children = stmt.get_children();
  auto if_block = add_basic_block();
  auto finish_block = add_basic_block();

  bool has_else = children.size() > 5;
  std::shared_ptr<BasicBlock> else_block = nullptr;
  if (has_else) {
    else_block = add_basic_block();
  }

  true_basic_blocks.push_back(if_block);
  false_basic_blocks.push_back(has_else ? else_block : finish_block);
  children[2]->accept(*this);
  true_basic_blocks.pop_back();
  false_basic_blocks.pop_back();

  current_basic_block = if_block;
  children[4]->accept(*this);
  current_basic_block->add_instructions(
      {Br::create(current_basic_block, finish_block)});

  if (has_else) {
    current_basic_block = else_block;
    children[6]->accept(*this);
    current_basic_block->add_instructions(
        {Br::create(current_basic_block, finish_block)});
  }

  current_basic_block = finish_block;
}

// Stmt ::= 'for' '(' [ForStmt] ';' [Cond] ';' [ForStmt] ')' Stmt
void IrBuilder::visit_for_stmt(const Stmt& stmt) {
  const auto& children = stmt.get_children();

  auto cond_block = add_basic_block();
  auto body_block = add_basic_block();
  auto update_block = add_basic_block();
  auto finish_block = add_basic_block();

  loop_entry_blocks.push_back(update_block);
  loop_exit_blocks.push_back(finish_block);

  size_t index = 2;
  if (children[index]->get_type() != NodeType::TOKEN) {  // ForStmt
    children[index]->accept(*this);
    ++index;
  }

  ++index;  // first semicolon
  current_basic_block->add_instructions(
      {Br::create(current_basic_block, cond_block)});

  current_basic_block = cond_block;
  if (children[index]->get_type() != NodeType::TOKEN) {  // Cond
    true_basic_blocks.push_back(body_block);
    false_basic_blocks.push_back(finish_block);
    children[index]->accept(*this);
    true_basic_blocks.pop_back();
    false_basic_blocks.pop_back();
    ++index;
  } else {
    current_basic_block->add_instructions(
        {Br::create(current_basic_block, body_block)});
  }

  current_basic_block = body_block;
  children.back()->accept(*this);  // loop body
  current_basic_block->add_instructions(
      {Br::create(current_basic_block, update_block)});

  current_basic_block = update_block;
  ++index;                                               // second semicolon
  if (children[index]->get_type() != NodeType::TOKEN) {  // ForStmt
    children[index]->accept(*this);
  }
  current_basic_block->add_instructions(
      {Br::create(current_basic_block, cond_block)});

  current_basic_block = finish_block;
  loop_entry_blocks.pop_back();
  loop_exit_blocks.pop_back();
}

void IrBuilder::visit_break_stmt(const Stmt& _) {
  current_basic_block->add_instructions(
      {Br::create(current_basic_block, loop_exit_blocks.back())});
  current_basic_block = add_basic_block();
}

void IrBuilder::visit_continue_stmt(const Stmt& _) {
  current_basic_block->add_instructions(
      {Br::create(current_basic_block, loop_entry_blocks.back())});
  current_basic_block = add_basic_block();
}

// Stmt ::= 'return' [Exp] ';'
void IrBuilder::visit_return_stmt(const Stmt& stmt) {
  const auto& children = stmt.get_children();
  if (children.size() == 2) {
    current_basic_block->add_instructions({Ret::create(current_basic_block)});
    return;
  }

  auto [ret_val, insts] = visit_exp(*children[1]);
  current_basic_block->add_instructions(std::move(insts));
  current_basic_block->add_instructions(
      {Ret::create(current_basic_block, ret_val)});
}

// Stmt ::= 'printf' '(' StringConst {',' Exp} ')' ';'
void IrBuilder::visit_printf_stmt(const Stmt& stmt) {
  const auto& children = stmt.get_children();
  std::string format = cast<TokenNode>(*children[2]).get_token().get_content();

  std::vector<std::shared_ptr<Value>> args;
  for (const auto& child : children) {
    if (child->get_type() != NodeType::EXP) {
      continue;
    }
    auto [arg, insts] = visit_exp(*child);
    current_basic_block->add_instructions(std::move(insts));
    args.push_back(arg);
  }

  auto emit_text = [this](const std::string& text) {
    if (text.empty()) {
      return;
    }

    if (text.size() == 1) {
      auto call = Call::create(current_basic_block, Function::putch,
                               {ConstantInt::create(text[0])});
      current_basic_block->add_instructions({call});
      return;
    }

    auto str_gv = get_or_create_string_constant(text);
    auto gep = Getelementptr::create(
        current_basic_block, str_gv,
        {ConstantInt::create(0), ConstantInt::create(0)}, gen_local_var_name());
    auto call = Call::create(current_basic_block, Function::putstr, {gep});
    current_basic_block->add_instructions({gep, call});
  };

  std::string text_buffer;
  size_t arg_index = 0;
  for (size_t i = 1; i + 1 < format.size(); ++i) {  // skip \"
    if (format[i] == '%' && i + 1 < format.size() - 1) {
      emit_text(text_buffer);
      text_buffer.clear();

      if (format[i + 1] == 'd') {
        auto call = Call::create(current_basic_block, Function::putint,
                                 {args[arg_index++]});
        current_basic_block->add_instructions({call});
        ++i;
        continue;
      }
    }

    if (format[i] == '\\' && i + 1 < format.size() - 1) {
      switch (format[i + 1]) {
        case 'n':
          text_buffer.push_back('\n');
          break;
        default:
          text_buffer.push_back(format[i + 1]);
          break;
      }
      ++i;
      continue;
    }

    text_buffer.push_back(format[i]);
  }

  emit_text(text_buffer);
}

}  // namespace midend::visitor

namespace midend::visitor {  // exp implementations
auto IrBuilder::visit_exp(const Node& node) -> ValueInsts {
  switch (node.get_type()) {
    case NodeType::EXP:
      return visit_exp(cast<Exp>(node));
    case NodeType::LVAL:
      return visit_exp(cast<LVal>(node));
    case NodeType::PRIMARY_EXP:
      return visit_exp(cast<PrimaryExp>(node));
    case NodeType::UNARY_EXP:
      return visit_exp(cast<UnaryExp>(node));
    case NodeType::MUL_EXP:
      return visit_exp(cast<MulExp>(node));
    case NodeType::ADD_EXP:
      return visit_exp(cast<AddExp>(node));
    case NodeType::REL_EXP:
      return visit_exp(cast<RelExp>(node));
    case NodeType::EQ_EXP:
      return visit_exp(cast<EqExp>(node));
    default:
      throw std::runtime_error("unsupported expression type");
  }
}

auto IrBuilder::visit_exp(const Exp& exp) -> ValueInsts {
  return visit_exp(child_as<AddExp>(exp, 0));
}

// LVal         ::= Ident [ '[' Exp ']' ]
auto IrBuilder::visit_exp(const LVal& lval) -> ValueInsts {
  auto symbol =
      symbol_table.get(Visitor::get_ident_name(child_as<Ident>(lval, 0)))
          .value();

  if (!symbol->has_ir_value()) {  // This can happen when the variable is
                                  // defined after its usage in the same scope
    auto upper =
        symbol_table.get(Visitor::get_ident_name(child_as<Ident>(lval, 0)), 1);
    ENSURE(upper.has_value() && (*upper)->has_ir_value(),
           "symbol has no IR value in visible scopes");
    symbol = *upper;
  }

  if (symbol->get_kind() == Symbol::SymbolKind::VAR) {
    if (!is_left.empty() && is_left.top()) {
      return {symbol->get_ir_value(), {}};
    }

    if (symbol->is_const() && std::dynamic_pointer_cast<Constant>(
                                  symbol->get_ir_value()) != nullptr) {
      return {symbol->get_ir_value(), {}};
    }

    auto load = Load::create(current_basic_block, symbol->get_ir_value(),
                             gen_local_var_name());
    return {load, {load}};
  }

  std::list<std::shared_ptr<Instruction>> insts;
  std::vector<std::shared_ptr<Value>> indexes;
  auto ptr = symbol->get_ir_value();
  auto pointee = std::static_pointer_cast<PointerType>(ptr->get_type())
                     ->get_reference_type();

  if (pointee->is_pointer_ty()) {
    // pointer to array, need to load the pointer first
    auto load = Load::create(current_basic_block, ptr, gen_local_var_name());
    insts.push_back(load);
    ptr = load;
  } else {
    indexes.push_back(ConstantInt::create(0));
  }

  for (const auto& child : lval.get_children()) {
    if (child->get_type() == NodeType::EXP) {
      auto [index_value, index_insts] = visit_exp(child_as<Exp>(lval, 2));
      insts.splice(insts.end(), index_insts);
      indexes.push_back(index_value);
    }
  }

  if (in_array_arg) {  // array passed as function argument, decay it to pointer
    indexes.push_back(ConstantInt::create(0));
  }

  auto gep = Getelementptr::create(current_basic_block, ptr, indexes,
                                   gen_local_var_name());
  insts.push_back(gep);

  if ((is_left.top()) || in_array_arg) {
    return {gep, insts};
  }

  auto load = Load::create(current_basic_block, gep, gen_local_var_name());
  insts.push_back(load);
  return {load, insts};
}

auto IrBuilder::visit_exp(const PrimaryExp& primary_exp) -> ValueInsts {
  const auto& first = *primary_exp.get_children().front();
  if (first.get_type() == NodeType::TOKEN) {
    return visit_exp(child_as<Exp>(primary_exp, 1));
  }

  if (first.get_type() == NodeType::NUMBER) {
    return {
        ConstantInt::create(Visitor::extract_number_value(cast<Number>(first))),
        {}};
  }

  is_left.push(false);
  auto result = visit_exp(cast<LVal>(first));
  is_left.pop();
  return result;
}

auto IrBuilder::visit_exp(const UnaryExp& unary_exp) -> ValueInsts {
  const auto& children = unary_exp.get_children();
  if (children.size() == 1) {  // PrimaryExp
    return visit_exp(child_as<PrimaryExp>(unary_exp, 0));
  }

  if (children.size() == 2) {  // UnaryOp UnaryExp
    auto [value, insts] = visit_exp(*children[1]);
    auto op_type = child_as<TokenNode>(child_as<UnaryOp>(unary_exp, 0), 0)
                       .get_token()
                       .get_type();

    if (op_type == TokenType::PLUS) {
      return {value, insts};
    }

    if (op_type == TokenType::MINU) {
      auto sub = Sub::create(current_basic_block, get_zero(value->get_type()),
                             value, gen_local_var_name());
      insts.push_back(sub);
      return {sub, insts};
    }

    if (op_type == TokenType::NOT) {
      // Convert to i32 first
      auto [converted, conv_insts] =
          make_type_conversion(value, IntegerType::get(32));
      insts.splice(insts.end(), conv_insts);
      auto cmp =
          ICmp::create(current_basic_block, ICmp::ICmpType::EQ,
                       ConstantInt::create(0), converted, gen_local_var_name());
      insts.push_back(cmp);
      return {cmp, insts};
    }

    throw std::runtime_error("unsupported unary operator");
  }

  auto symbol_opt =
      symbol_table.get(Visitor::get_ident_name(cast<Ident>(*children[0])));
  auto func_symbol = std::static_pointer_cast<FuncSymbol>(*symbol_opt);
  auto function =
      std::static_pointer_cast<Function>(func_symbol->get_ir_value());

  std::vector<std::shared_ptr<Value>> args;
  std::list<std::shared_ptr<Instruction>> insts;
  if (children.size() > 3) {  // function call with arguments
    auto param_types =
        std::static_pointer_cast<FunctionType>(function->get_type())
            ->get_param_types();

    size_t arg_index = 0;
    for (const auto& param : children[2]->get_children()) {
      if (param->get_type() == NodeType::TOKEN) {
        continue;
      }

      in_array_arg = param_types[arg_index]->is_pointer_ty();
      auto [arg, arg_insts] = visit_exp(*param);
      insts.splice(insts.end(), arg_insts);
      args.push_back(arg);
      in_array_arg = false;
      ++arg_index;
    }
  }

  auto call =
      Call::create(current_basic_block, function, args, gen_local_var_name());
  insts.push_back(call);
  return {call, insts};
}

auto IrBuilder::visit_exp(const MulExp& mul_exp) -> ValueInsts {
  auto [value, insts] = visit_exp(child_as<UnaryExp>(mul_exp, 0));
  const auto& children = mul_exp.get_children();

  for (size_t i = 1; i < children.size(); i += 2) {
    auto [rhs, rhs_insts] = visit_exp(cast<UnaryExp>(*children[i + 1]));
    insts.splice(insts.end(), rhs_insts);

    auto op = cast<TokenNode>(*children[i]).get_token().get_type();

    auto [lhs_conv, rhs_conv, conv_insts] =
        make_binary_type_conversion(value, rhs);
    insts.splice(insts.end(), conv_insts);
    value = lhs_conv;
    rhs = rhs_conv;

    std::shared_ptr<Instruction> inst;
    switch (op) {
      case TokenType::MULT:
        inst =
            Mul::create(current_basic_block, value, rhs, gen_local_var_name());
        break;
      case TokenType::DIV:
        inst =
            SDiv::create(current_basic_block, value, rhs, gen_local_var_name());
        break;
      case TokenType::MOD:
        inst =
            SRem::create(current_basic_block, value, rhs, gen_local_var_name());
        break;
      default:
        throw std::runtime_error("unsupported mul operator");
    }

    value = inst;
    insts.push_back(inst);
  }

  return {value, insts};
}

auto IrBuilder::visit_exp(const AddExp& add_exp) -> ValueInsts {
  auto [value, insts] = visit_exp(child_as<MulExp>(add_exp, 0));
  const auto& children = add_exp.get_children();

  for (size_t i = 1; i < children.size(); i += 2) {
    auto [rhs, rhs_insts] = visit_exp(cast<MulExp>(*children[i + 1]));
    insts.splice(insts.end(), rhs_insts);

    auto op = cast<TokenNode>(*children[i]).get_token().get_type();

    auto [lhs_conv, rhs_conv, conv_insts] =
        make_binary_type_conversion(value, rhs);
    insts.splice(insts.end(), conv_insts);
    value = lhs_conv;
    rhs = rhs_conv;

    std::shared_ptr<Instruction> inst;
    if (op == TokenType::PLUS) {
      inst = Add::create(current_basic_block, value, rhs, gen_local_var_name());
    } else if (op == TokenType::MINU) {
      inst = Sub::create(current_basic_block, value, rhs, gen_local_var_name());
    } else {
      throw std::runtime_error("unsupported add operator");
    }

    value = inst;
    insts.push_back(inst);
  }

  return {value, insts};
}

auto IrBuilder::visit_exp(const RelExp& rel_exp) -> ValueInsts {
  auto [value, insts] = visit_exp(child_as<AddExp>(rel_exp, 0));
  const auto& children = rel_exp.get_children();

  for (size_t i = 1; i < children.size(); i += 2) {
    auto [rhs, rhs_insts] = visit_exp(cast<AddExp>(*children[i + 1]));
    insts.splice(insts.end(), rhs_insts);

    auto op = cast<TokenNode>(*children[i]).get_token().get_type();

    auto [lhs_conv, rhs_conv, conv_insts] =
        make_binary_type_conversion(value, rhs);
    insts.splice(insts.end(), conv_insts);

    ICmp::ICmpType cmp_type;
    switch (op) {
      case TokenType::LSS:
        cmp_type = ICmp::ICmpType::SLT;
        break;
      case TokenType::LEQ:
        cmp_type = ICmp::ICmpType::SLE;
        break;
      case TokenType::GRE:
        cmp_type = ICmp::ICmpType::SGT;
        break;
      case TokenType::GEQ:
        cmp_type = ICmp::ICmpType::SGE;
        break;
      default:
        throw std::runtime_error("unsupported rel operator");
    }

    auto cmp = ICmp::create(current_basic_block, cmp_type, lhs_conv, rhs_conv,
                            gen_local_var_name());
    value = cmp;
    insts.push_back(cmp);
  }

  return {value, insts};
}

auto IrBuilder::visit_exp(const EqExp& eq_exp) -> ValueInsts {
  auto [value, insts] = visit_exp(child_as<RelExp>(eq_exp, 0));
  const auto& children = eq_exp.get_children();

  for (size_t i = 1; i < children.size(); i += 2) {
    auto [rhs, rhs_insts] = visit_exp(cast<RelExp>(*children[i + 1]));
    insts.splice(insts.end(), rhs_insts);

    auto op = cast<TokenNode>(*children[i]).get_token().get_type();

    auto [lhs_conv, rhs_conv, conv_insts] =
        make_binary_type_conversion(value, rhs);
    insts.splice(insts.end(), conv_insts);

    ICmp::ICmpType cmp_type =
        (op == TokenType::EQL) ? ICmp::ICmpType::EQ : ICmp::ICmpType::NE;
    auto cmp = ICmp::create(current_basic_block, cmp_type, lhs_conv, rhs_conv,
                            gen_local_var_name());
    value = cmp;
    insts.push_back(cmp);
  }

  return {value, insts};
}
}  // namespace midend::visitor

namespace midend::visitor {  // helper functions
auto IrBuilder::get_ir_type(const SymbolPtr& symbol) -> std::shared_ptr<Type> {
  switch (symbol->get_kind()) {
    case Symbol::SymbolKind::VAR: {
      return IntegerType::get(32);
    }
    case Symbol::SymbolKind::ARRAY: {
      return PointerType::get(IntegerType::get(32));
    }
    case Symbol::SymbolKind::FUNC: {
      auto func = std::static_pointer_cast<FuncSymbol>(symbol);
      std::shared_ptr<Type> return_type =
          func->returns_value()
              ? std::static_pointer_cast<Type>(IntegerType::get(32))
              : std::static_pointer_cast<Type>(VoidType::get());
      std::vector<std::shared_ptr<Type>> param_types;
      for (const auto& param_symbol : func->get_param_list()) {
        param_types.push_back(get_ir_type(param_symbol));
      }
      return FunctionType::get(return_type, param_types);
    }
    default:
      throw std::runtime_error("unsupported symbol kind in get_ir_type");
  }
}

auto IrBuilder::make_type_conversion(const std::shared_ptr<Value>& val,
                                     const std::shared_ptr<Type>& target_type)
    -> ValueInsts {
  if (val->get_type() == target_type) {
    return {val, {}};
  }

  if (!val->get_type()->is_integer_ty() || !target_type->is_integer_ty()) {
    throw std::runtime_error("only integer conversion is supported");
  }

  auto source_bits = val->get_type()->bits_num();
  auto target_bits = target_type->bits_num();

  if (target_bits == 1) {
    auto cmp = ICmp::create(current_basic_block, ICmp::ICmpType::NE,
                            ConstantInt::create(0, source_bits), val,
                            gen_local_var_name());
    return {cmp, {cmp}};
  }

  if (source_bits < target_bits) {
    auto zext = ZExt::create(current_basic_block, val, target_type,
                             gen_local_var_name());
    return {zext, {zext}};
  }

  auto trunc = Trunc::create(current_basic_block, val, target_type,
                             gen_local_var_name());
  return {trunc, {trunc}};
}

auto IrBuilder::make_binary_type_conversion(const std::shared_ptr<Value>& left,
                                            const std::shared_ptr<Value>& right)
    -> std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>,
                  std::list<std::shared_ptr<Instruction>>> {
  if (left->get_type() == right->get_type()) {
    return {left, right, {}};
  }

  std::list<std::shared_ptr<Instruction>> insts;
  auto left_bits = left->get_type()->bits_num();
  auto right_bits = right->get_type()->bits_num();

  if (left_bits == 1) {
    auto [extended, ext_insts] =
        make_type_conversion(left, IntegerType::get(right_bits));
    insts.splice(insts.end(), ext_insts);
    return {extended, right, insts};
  }

  if (right_bits == 1) {
    auto [extended, ext_insts] =
        make_type_conversion(right, IntegerType::get(left_bits));
    insts.splice(insts.end(), ext_insts);
    return {left, extended, insts};
  }

  throw std::runtime_error("unsupported binary type conversion");
}

auto IrBuilder::add_global_var(std::shared_ptr<Type> type,
                               const std::shared_ptr<Constant>& init_value)
    -> std::shared_ptr<GlobalVariable> {
  auto gv =
      std::make_shared<GlobalVariable>(type, init_value, gen_global_var_name());
  module.add_global_variable(gv);
  return gv;
}

auto IrBuilder::add_func(const std::string& name,
                         const std::shared_ptr<Type>& func_type)
    -> std::shared_ptr<Function> {
  auto func = std::make_shared<Function>(func_type, get_func_name(name));
  module.add_function(func);
  current_function = func;
  auto bb = add_basic_block();
  current_basic_block = bb;
  return func;
}

auto IrBuilder::add_basic_block() -> std::shared_ptr<BasicBlock> {
  auto bb = std::make_shared<BasicBlock>(current_function, gen_block_name());
  current_function->add_basic_block(bb);
  return bb;
}

auto IrBuilder::create_entry_alloca(const std::shared_ptr<Type>& content_type)
    -> std::shared_ptr<Value> {
  auto entry_block = current_function->entry_block();
  ENSURE(entry_block != nullptr, "Cannot allocate without a function entry");

  auto alloca_inst =
      Alloca::create(entry_block, content_type, gen_local_var_name());
  auto& instructions = entry_block->get_instructions();
  auto insertion_point = std::find_if(
      instructions.begin(), instructions.end(), [](const auto& instruction) {
        return instruction->get_instruction_type() !=
               Instruction::InstructionType::ALLOCA;
      });
  instructions.insert(insertion_point, alloca_inst);
  return alloca_inst;
}

auto IrBuilder::get_or_create_string_constant(const std::string& str)
    -> std::shared_ptr<GlobalVariable> {
  auto cached = string_constants_cache.find(str);
  if (cached != string_constants_cache.end()) {
    return cached->second;
  }

  auto str_const = ConstantString::create(str);
  auto gv = add_global_var(PointerType::get(str_const->get_type()), str_const);
  string_constants_cache.emplace(str, gv);
  return gv;
}

}  // namespace midend::visitor

namespace midend::visitor {  // evaluate constant
auto IrBuilder::eval_const_init(const ConstInitVal& const_init_val) const
    -> std::vector<std::shared_ptr<Constant>> {
  std::vector<std::shared_ptr<Constant>> result;
  const auto& children = const_init_val.get_children();
  for (const auto& child : children) {
    if (child->get_type() == NodeType::CONST_EXP) {
      result.push_back(eval_exp(cast<ConstExp>(*child)));
    }
  }
  return result;
}

auto IrBuilder::eval_init(const InitVal& init_val) const
    -> std::vector<std::shared_ptr<Constant>> {
  std::vector<std::shared_ptr<Constant>> result;
  const auto& children = init_val.get_children();
  for (const auto& child : children) {
    if (child->get_type() == NodeType::EXP) {
      auto const_val = eval_exp(cast<Exp>(*child));
      ENSURE(const_val != nullptr,
             "failed to evaluate constant expression in init value");
      result.push_back(const_val);
    }
  }
  return result;
}

auto IrBuilder::eval_exp(const ConstExp& const_exp) const
    -> std::shared_ptr<Constant> {
  auto res = eval_exp(child_as<AddExp>(const_exp, 0));
  ENSURE(res, "failed to evaluate constant expression");
  return res;
}

auto IrBuilder::eval_exp(const Exp& exp) const -> std::shared_ptr<Constant> {
  return eval_exp(child_as<AddExp>(exp, 0));
}

auto IrBuilder::eval_exp(const AddExp& add_exp) const
    -> std::shared_ptr<Constant> {
  auto lhs = eval_exp(child_as<MulExp>(add_exp, 0));
  for (size_t i = 2; i < add_exp.get_children().size(); i += 2) {
    auto rhs = eval_exp(child_as<MulExp>(add_exp, i));
    auto op = child_as<TokenNode>(add_exp, i - 1).get_token().get_type();
    lhs = cal_binary_exp(lhs, rhs, op);
  }
  return lhs;
}

auto IrBuilder::eval_exp(const MulExp& mul_exp) const
    -> std::shared_ptr<Constant> {
  auto lhs = eval_exp(child_as<UnaryExp>(mul_exp, 0));
  for (size_t i = 2; i < mul_exp.get_children().size(); i += 2) {
    auto rhs = eval_exp(child_as<UnaryExp>(mul_exp, i));
    auto op = child_as<TokenNode>(mul_exp, i - 1).get_token().get_type();
    lhs = cal_binary_exp(lhs, rhs, op);
  }
  return lhs;
}

// UnaryExp ::= PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
auto IrBuilder::eval_exp(const UnaryExp& unary_exp) const
    -> std::shared_ptr<Constant> {
  auto& first = unary_exp.get_children().front();
  if (first->get_type() == NodeType::IDENT) {
    throw std::runtime_error("function call is not a constant expression");
  }

  if (first->get_type() == NodeType::PRIMARY_EXP) {
    return eval_exp(cast<PrimaryExp>(*first));
  }

  auto op = child_as<TokenNode>(child_as<UnaryOp>(unary_exp, 0), 0)
                .get_token()
                .get_type();
  auto value = eval_exp(child_as<UnaryExp>(unary_exp, 1));
  ENSURE(value != nullptr, "failed to evaluate unary expression as constant");
  switch (op) {
    case TokenType::PLUS:
      return value;
    case TokenType::MINU:
      return -(*value);
    default:
      throw std::runtime_error(
          "unsupported unary operator in constant expression");
  }
}

auto IrBuilder::eval_exp(const PrimaryExp& primary_exp) const
    -> std::shared_ptr<Constant> {
  using NodeType = frontend::ast::NodeType;
  const auto& first = *primary_exp.get_children().front();
  switch (first.get_type()) {
    case NodeType::TOKEN:
      return eval_exp(child_as<Exp>(primary_exp, 1));
    case NodeType::NUMBER:
      return eval_exp(cast<Number>(first));
    case NodeType::LVAL:
      return eval_exp(cast<LVal>(first));
    default:
      throw std::runtime_error(
          "unexpected primary expression in constant expression");
  }
}

// LVal → Ident ['[' Exp ']']
auto IrBuilder::eval_exp(const LVal& lval) const -> std::shared_ptr<Constant> {
  auto name = Visitor::get_ident_name(child_as<Ident>(lval, 0));
  const auto& children = lval.get_children();
  auto symbol = symbol_table.get(name).value();
  if (!symbol->has_ir_value()) {  // This can happen when the variable is
                                  // defined after its usage in the same scope
    auto upper =
        symbol_table.get(Visitor::get_ident_name(child_as<Ident>(lval, 0)), 1);
    ENSURE(upper.has_value() && (*upper)->has_ir_value(),
           "symbol has no IR value in visible scopes");
    symbol = *upper;
  }
  ENSURE(symbol->is_const(), "LVal:" + name + " symbol is not const");
  auto ir_value = symbol->get_ir_value();

  if (children.size() == 1) {
    return std::dynamic_pointer_cast<Constant>(symbol->get_ir_value());
  }

  // array element access
  auto index_const =
      std::static_pointer_cast<ConstantInt>(eval_exp(cast<Exp>(*children[2])));
  ENSURE(index_const != nullptr, "failed to evaluate array index as constant");
  auto index = index_const->get_val();

  std::shared_ptr<ConstantArray> const_array =
      std::dynamic_pointer_cast<ConstantArray>(ir_value);
  if (const_array == nullptr) {
    if (auto gv = std::dynamic_pointer_cast<GlobalVariable>(ir_value)) {
      const_array =
          std::dynamic_pointer_cast<ConstantArray>(gv->get_init_value());
    }
  }

  ENSURE(const_array != nullptr && index >= 0 &&
             static_cast<size_t>(index) < const_array->get_values().size(),
         "invalid array index in constant expression");
  return const_array->get_values()[index];
}

auto IrBuilder::eval_exp(const Number& number) const
    -> std::shared_ptr<Constant> {
  return ConstantInt::create(Visitor::extract_number_value(number));
}

auto IrBuilder::cal_binary_exp(const std::shared_ptr<Constant>& lhs,
                               const std::shared_ptr<Constant>& rhs,
                               TokenType op) const
    -> std::shared_ptr<Constant> {
  ENSURE(lhs != nullptr && rhs != nullptr,
         "failed to evaluate binary expression as constant");

  switch (op) {
    case TokenType::PLUS:
      return *lhs + *rhs;
    case TokenType::MINU:
      return *lhs - *rhs;
    case TokenType::MULT:
      return *lhs * *rhs;
    case TokenType::DIV:
      return *lhs / *rhs;
    case TokenType::MOD:
      return *lhs % *rhs;
    default:
      throw std::runtime_error(
          "unsupported binary operator in constant expression");
  }
}

}  // namespace midend::visitor
