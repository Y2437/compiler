#ifndef BUAA_COMPILER_SYMTABLE
#define BUAA_COMPILER_SYMTABLE

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace midend::llvm {
class Value;
}

// output symbol table
namespace midend::symbol {
#define TYPE_PRINT_TABLE            \
  X(INT, Int)                       \
  X(CONST_INT, ConstInt)            \
  X(STATIC_INT, StaticInt)          \
                                    \
  X(INT_ARR, IntArray)              \
  X(CONST_INT_ARR, ConstIntArray)   \
  X(STATIC_INT_ARR, StaticIntArray) \
                                    \
  X(VOID_FUNC, VoidFunc)            \
  X(INT_FUNC, IntFunc)

enum class SymbolPrintType {
#define X(type, name) type,
  TYPE_PRINT_TABLE
#undef X
      COUNT,
};

std::ostream& operator<<(std::ostream& os, SymbolPrintType type);

}  // namespace midend::symbol

namespace midend::symbol {
class Symbol;
using SymbolPtr = std::shared_ptr<Symbol>;

class Symbol {
 public:
  enum class SymbolKind { VAR, ARRAY, FUNC };

  virtual ~Symbol() = default;
  virtual SymbolPrintType get_symbol_print_type() const = 0;
  void set_present_index(size_t index) { present_index = index; }
  auto get_present_index() const -> size_t { return present_index; }
  void set_name(const std::string& name) { this->name = name; }
  auto get_name() const -> std::string { return name; }
  auto get_kind() const -> SymbolKind { return kind; }

  bool is_const() const { return _is_const; }
  bool is_static() const { return _is_static; }
  void set_ir_value(const std::shared_ptr<midend::llvm::Value>& value) {
    ir_value = value;
  }
  auto get_ir_value() const -> std::shared_ptr<midend::llvm::Value> {
    return ir_value;
  }
  bool has_ir_value() const { return ir_value != nullptr; }

 protected:
  bool _is_const = false;
  bool _is_static = false;

  explicit Symbol(SymbolKind kind) : kind(kind) {}
  Symbol(SymbolKind kind, bool is_const, bool is_static)
      : _is_const(is_const), _is_static(is_static), kind(kind) {}

 private:
  SymbolKind kind;
  size_t present_index;  // the order of declaration in the whole program
  std::string name;
  std::shared_ptr<midend::llvm::Value> ir_value;
};

class VarSymbol : public Symbol {
 public:
  VarSymbol(bool is_const, bool is_static)
      : Symbol(SymbolKind::VAR, is_const, is_static) {}

  SymbolPrintType get_symbol_print_type() const override;
};

class ArraySymbol : public Symbol {
 public:
  ArraySymbol(bool is_const, bool is_static)
      : Symbol(SymbolKind::ARRAY, is_const, is_static) {}

  SymbolPrintType get_symbol_print_type() const override;
};

class FuncSymbol : public Symbol {
 public:
  explicit FuncSymbol(bool returns_value)
      : Symbol(SymbolKind::FUNC), returns_value_(returns_value) {}

  void set_param_list(std::vector<SymbolPtr>&& param_list) {
    this->param_list = std::move(param_list);
  }
  auto& get_param_list() { return param_list; }
  bool returns_value() const { return returns_value_; }

  SymbolPrintType get_symbol_print_type() const override;

 private:
  bool returns_value_;
  std::vector<SymbolPtr> param_list;
};

class SymbolTable {
 public:
  using Scope =
      std::pair<std::size_t, std::unordered_map<std::string, SymbolPtr>>;
  SymbolTable();

  void enter_scope();
  void exit_scope();
  bool current_scope_contains(const std::string& name) const;
  void insert(const std::string& name, SymbolPtr symbol);

  // offset = 0 means current scope, offset = 1 means one level up, etc.
  auto get(const std::string& name, int offset = 0) const
      -> std::optional<SymbolPtr>;

  void insert_main();

  bool is_global() const;

  void output_symbol_table(std::ostream& os) const;

  // ========== for ir builder replay ==========

  void prepare_replay();
  void replay_enter_scope();
  void replay_exit_scope();

 private:
  size_t scope_level = 0;
  // just for passing tests
  size_t present_counter = 0;
  std::vector<std::unique_ptr<Scope>> scopes;
  // Semantic analysis records exited scopes here. IR generation then replays
  // them in the same order, keeping symbol state out of the AST itself.
  std::vector<std::unique_ptr<Scope>> popped_scopes;
};

}  // namespace midend::symbol

#endif
