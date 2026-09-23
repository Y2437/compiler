#ifndef BUAA_COMPILER_LLVM_VALUE
#define BUAA_COMPILER_LLVM_VALUE

#include <array>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "midend/llvm/type.hpp"
#include "midend/opt/support.hpp"

namespace midend::llvm {

class Value;
class User;
class Function;
class Argument;
class GlobalVariable;
class BasicBlock;
class Instruction;
class Constant;
class ConstantInt;

class Value {
 public:
  explicit Value(const std::shared_ptr<Type>& type, std::string name = "");
  virtual ~Value() = default;

  auto get_type() const -> std::shared_ptr<Type> { return type; }
  auto get_name() const -> const std::string& { return name; }
  auto get_id() const -> int { return id; }

  void set_name(std::string new_name) { name = std::move(new_name); }
  void set_type(const std::shared_ptr<Type>& new_type) { type = new_type; }

  void add_user(const std::shared_ptr<User>& user);
  void remove_user(const std::shared_ptr<User>& user);

  auto get_users() -> std::vector<std::weak_ptr<User>>& { return users; }
  auto get_users() const -> const std::vector<std::weak_ptr<User>>& {
    return users;
  }

  virtual auto to_string() const -> std::string = 0;

 protected:
  int id;
  std::string name;
  std::shared_ptr<Type> type;
  std::vector<std::weak_ptr<User>> users;

 private:
  static auto generate_id() -> int;
};

class User : public Value {
 public:
  explicit User(const std::shared_ptr<Type>& type, std::string name = "")
      : Value(type, std::move(name)) {}

  void add_operand(const std::shared_ptr<Value>& operand) {
    operands.push_back(operand);
  }

  auto get_operand(size_t index) const -> std::shared_ptr<Value> {
    return operands.at(index);
  }
  void set_operand(size_t index, const std::shared_ptr<Value>& value) {
    operands.at(index) = value;
  }

  auto get_operands() -> std::vector<std::shared_ptr<Value>>& {
    return operands;
  }
  auto get_operands() const -> const std::vector<std::shared_ptr<Value>>& {
    return operands;
  }
  auto get_num_operands() const -> size_t { return operands.size(); }

 protected:
  std::vector<std::shared_ptr<Value>> operands;
};

class Function : public Value {
 public:
  explicit Function(const std::shared_ptr<Type>& type, std::string name = "")
      : Value(type, std::move(name)) {}

  void add_argument(const std::shared_ptr<Argument>& arg) {
    arguments.push_back(arg);
  }
  void set_arguments(std::vector<std::shared_ptr<Argument>> args) {
    arguments = std::move(args);
  }

  void add_basic_block(const std::shared_ptr<BasicBlock>& bb) {
    basic_blocks.push_back(bb);
  }

  auto get_arguments() const -> const std::vector<std::shared_ptr<Argument>>& {
    return arguments;
  }

  auto get_basic_blocks() const
      -> const std::list<std::shared_ptr<BasicBlock>>& {
    return basic_blocks;
  }

  auto get_basic_blocks() -> std::list<std::shared_ptr<BasicBlock>>& {
    return basic_blocks;
  }
  auto get_basic_blocks_ref() -> std::list<std::shared_ptr<BasicBlock>>& {
    return basic_blocks;
  }

  auto get_return_type() const -> std::shared_ptr<Type>;
  auto get_param_types() const -> std::vector<std::shared_ptr<Type>>;
  auto entry_block() const -> std::shared_ptr<BasicBlock> {
    return basic_blocks.empty() ? nullptr : basic_blocks.front();
  }

  auto is_main() const -> bool { return name == "@main"; }
  auto to_string() const -> std::string override;

  static std::shared_ptr<Function> getint;
  static std::shared_ptr<Function> putint;
  static std::shared_ptr<Function> putch;
  static std::shared_ptr<Function> putstr;

  static auto get_lib_funcs()
      -> const std::array<std::shared_ptr<Function>, 4>& {
    static const std::array<std::shared_ptr<Function>, 4> lib_funcs = {
        getint, putint, putch, putstr};
    return lib_funcs;
  }

 private:
  std::vector<std::shared_ptr<Argument>> arguments;
  std::list<std::shared_ptr<BasicBlock>> basic_blocks;
};

class GlobalVariable : public Value {
 public:
  GlobalVariable(const std::shared_ptr<Type>& type,
                 const std::shared_ptr<Constant>& init_value,
                 std::string name = "")
      : Value(type, std::move(name)), init_value(init_value) {}

  auto get_init_value() const -> std::shared_ptr<Constant> {
    return init_value;
  }
  auto to_string() const -> std::string override;

 private:
  std::shared_ptr<Constant> init_value;
};

class Argument : public Value {
 public:
  Argument(const std::shared_ptr<Type>& type,
           const std::shared_ptr<Function>& parent_func, std::string name = "")
      : Value(type, std::move(name)), parent_func(parent_func) {}

  auto to_string() const -> std::string override;

 private:
  std::weak_ptr<Function> parent_func;
};

class BasicBlock : public Value {
 public:
  explicit BasicBlock(const std::shared_ptr<Function>& parent_func,
                      std::string name = "")
      : Value(LabelType::get(), std::move(name)), parent_func(parent_func) {}

  void add_instructions(
      std::list<std::shared_ptr<Instruction>> instructions_to_add);

  auto get_instructions() -> std::list<std::shared_ptr<Instruction>>& {
    return instructions;
  }
  auto get_instructions_ref() -> std::list<std::shared_ptr<Instruction>>& {
    return instructions;
  }

  auto get_instructions() const
      -> const std::list<std::shared_ptr<Instruction>>& {
    return instructions;
  }

  void append_instruction(const std::shared_ptr<Instruction>& inst) {
    instructions.push_back(inst);
  }
  void prepend_instruction(const std::shared_ptr<Instruction>& inst) {
    instructions.push_front(inst);
  }

  auto get_terminator() const -> std::shared_ptr<Instruction>;

  auto get_parent_func() const -> std::weak_ptr<Function> {
    return parent_func;
  }
  void set_parent_func(const std::shared_ptr<Function>& func) {
    parent_func = func;
  }

  auto to_string() const -> std::string override;

  opt::BasicBlockOptInfo opt_info;

 private:
  std::list<std::shared_ptr<Instruction>> instructions;
  std::weak_ptr<Function> parent_func;
};

class Instruction : public User {
 public:
  enum class InstructionType {
    RET,
    BR,
    ADD,
    SUB,
    MUL,
    SDIV,
    SREM,
    AND,
    OR,
    XOR,
    SHL,
    LSHR,
    ASHR,
    ALLOCA,
    LOAD,
    STORE,
    GETELEMENTPTR,
    TRUNC,
    ZEXT,
    ICMP,
    CALL,
    PHI,
    PHICOPY,
    MOVE,
  };

  explicit Instruction(const std::shared_ptr<Type>& type,
                       InstructionType ins_type,
                       const std::shared_ptr<BasicBlock>& parent_block,
                       std::string name = "")
      : User(type, std::move(name)),
        ins_type(ins_type),
        parent_block(parent_block) {}

  virtual ~Instruction() = default;

  auto get_instruction_type() const -> InstructionType { return ins_type; }
  auto get_parent_block() const -> std::weak_ptr<BasicBlock> {
    return parent_block;
  }
  void set_parent_block(const std::shared_ptr<BasicBlock>& block) {
    parent_block = block;
  }

 protected:
  InstructionType ins_type;
  std::weak_ptr<BasicBlock> parent_block;
};

extern const std::unordered_map<Instruction::InstructionType, std::string>
    BINARY_INS_TYPE_TO_STR;
extern const std::unordered_map<Instruction::InstructionType, std::string>
    CONVERSION_INS_TYPE_TO_STR;

class Constant : public Value {
 public:
  explicit Constant(const std::shared_ptr<Type>& type, std::string name)
      : Value(type, std::move(name)) {}

  virtual auto operator+(const Constant& rhs) const
      -> std::shared_ptr<Constant>;
  virtual auto operator-(const Constant& rhs) const
      -> std::shared_ptr<Constant>;
  virtual auto operator*(const Constant& rhs) const
      -> std::shared_ptr<Constant>;
  virtual auto operator/(const Constant& rhs) const
      -> std::shared_ptr<Constant>;
  virtual auto operator%(const Constant& rhs) const
      -> std::shared_ptr<Constant>;
  virtual auto operator-() const -> std::shared_ptr<Constant>;
};

class ConstantInt : public Constant {
 public:
  ConstantInt(const std::shared_ptr<Type>& type, int val)
      : Constant(type, std::to_string(val)), val(val) {}

  static auto create(int val, int bitnum = 32) -> std::shared_ptr<ConstantInt>;

  auto get_val() const -> int { return val; }
  auto to_string() const -> std::string override { return name; }

  auto operator+(const Constant& rhs) const
      -> std::shared_ptr<Constant> override;
  auto operator-(const Constant& rhs) const
      -> std::shared_ptr<Constant> override;
  auto operator*(const Constant& rhs) const
      -> std::shared_ptr<Constant> override;
  auto operator/(const Constant& rhs) const
      -> std::shared_ptr<Constant> override;
  auto operator%(const Constant& rhs) const
      -> std::shared_ptr<Constant> override;
  auto operator-() const -> std::shared_ptr<Constant> override;

 private:
  int val;
};

class ConstantString : public Constant {
 public:
  ConstantString(const std::string& str, std::string name = "",
                 bool add_null = true)
      : Constant(
            ArrayType::get(IntegerType::get(8),
                           static_cast<int>(str.size() + (add_null ? 1 : 0))),
            std::move(name)),
        str_value(str),
        add_null(add_null) {}

  static auto create(const std::string& str, std::string name = "",
                     bool add_null = true) -> std::shared_ptr<ConstantString>;

  auto get_string_value() const -> const std::string& { return str_value; }
  auto has_null_terminator() const -> bool { return add_null; }

  auto to_string() const -> std::string override;

 private:
  std::string str_value;
  bool add_null;
};

class ConstantArray : public Constant {
 public:
  ConstantArray(const std::shared_ptr<Type>& type,
                std::vector<std::shared_ptr<Constant>> vals,
                std::string name = "")
      : Constant(type, std::move(name)), vals(std::move(vals)) {}

  auto get_values() const -> const std::vector<std::shared_ptr<Constant>>& {
    return vals;
  }

  auto to_string() const -> std::string override;

 private:
  std::vector<std::shared_ptr<Constant>> vals;
};

class ZeroInitializer : public Constant {
 public:
  explicit ZeroInitializer(const std::shared_ptr<Type>& type)
      : Constant(type, "zeroinitializer") {}

  auto to_string() const -> std::string override { return name; }
};

inline auto get_zero(const std::shared_ptr<Type>& type)
    -> std::shared_ptr<Constant> {
  if (type->is_integer_ty()) {
    return ConstantInt::create(0, type->bits_num());
  }
  if (type->is_array_ty()) {
    return std::make_shared<ZeroInitializer>(type);
  }
  throw std::runtime_error("unsupported type for zero initializer");
}

class Placeholder : public Value {
 public:
  explicit Placeholder(const std::shared_ptr<Type>& type, std::string name = "")
      : Value(type, std::move(name)) {}

  auto to_string() const -> std::string override { return name; }
};

}  // namespace midend::llvm

#endif  // BUAA_COMPILER_LLVM_VALUE
