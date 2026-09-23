#include "midend/llvm/value.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>

namespace midend::llvm {

auto Value::generate_id() -> int {
  static int counter = 0;
  return counter++;
}

Value::Value(const std::shared_ptr<Type>& type, std::string name)
    : id(generate_id()), name(std::move(name)), type(type) {}

void Value::add_user(const std::shared_ptr<User>& user) {
  auto it = std::find_if(
      users.begin(), users.end(),
      [&user](const std::weak_ptr<User>& u) { return u.lock() == user; });
  if (it == users.end()) {
    users.push_back(user);
  }
}

void Value::remove_user(const std::shared_ptr<User>& user) {
  users.erase(std::remove_if(users.begin(), users.end(),
                             [&user](const std::weak_ptr<User>& u) {
                               auto locked = u.lock();
                               return !locked || locked == user;
                             }),
              users.end());
}

void BasicBlock::add_instructions(
    std::list<std::shared_ptr<Instruction>> instructions_to_add) {
  instructions.splice(instructions.end(), instructions_to_add);
}

auto BasicBlock::get_terminator() const -> std::shared_ptr<Instruction> {
  if (instructions.empty()) {
    return nullptr;
  }

  const auto& last = instructions.back();
  const auto ins_type = last->get_instruction_type();
  if (ins_type == Instruction::InstructionType::RET ||
      ins_type == Instruction::InstructionType::BR) {
    return last;
  }
  return nullptr;
}

auto Function::get_return_type() const -> std::shared_ptr<Type> {
  return std::static_pointer_cast<FunctionType>(type)->get_return_type();
}

auto Function::get_param_types() const -> std::vector<std::shared_ptr<Type>> {
  return std::static_pointer_cast<FunctionType>(type)->get_param_types();
}

using InstructionType = Instruction::InstructionType;

const std::unordered_map<InstructionType, std::string> BINARY_INS_TYPE_TO_STR =
    {
        {InstructionType::RET, "ret"},
        {InstructionType::BR, "br"},
        {InstructionType::ADD, "add"},
        {InstructionType::SUB, "sub"},
        {InstructionType::MUL, "mul"},
        {InstructionType::SDIV, "sdiv"},
        {InstructionType::SREM, "srem"},
        {InstructionType::ALLOCA, "alloca"},
        {InstructionType::LOAD, "load"},
        {InstructionType::STORE, "store"},
        {InstructionType::GETELEMENTPTR, "getelementptr"},
        {InstructionType::TRUNC, "trunc"},
        {InstructionType::ZEXT, "zext"},
        {InstructionType::ICMP, "icmp"},
        {InstructionType::CALL, "call"},
};

const std::unordered_map<InstructionType, std::string>
    CONVERSION_INS_TYPE_TO_STR = {
        {InstructionType::TRUNC, "trunc"},
        {InstructionType::ZEXT, "zext"},
};
}  // namespace midend::llvm

namespace midend::llvm {  // library functions

std::shared_ptr<Function> Function::getint = std::make_shared<Function>(
    FunctionType::get(IntegerType::get(32),
                      std::vector<std::shared_ptr<Type>>{}),
    "@getint");

std::shared_ptr<Function> Function::putint = std::make_shared<Function>(
    FunctionType::get(VoidType::get(),
                      std::vector<std::shared_ptr<Type>>{IntegerType::get(32)}),
    "@putint");

std::shared_ptr<Function> Function::putch = std::make_shared<Function>(
    FunctionType::get(VoidType::get(),
                      std::vector<std::shared_ptr<Type>>{IntegerType::get(32)}),
    "@putch");

std::shared_ptr<Function> Function::putstr = std::make_shared<Function>(
    FunctionType::get(VoidType::get(),
                      std::vector<std::shared_ptr<Type>>{
                          PointerType::get(IntegerType::get(8))}),
    "@putstr");
}  // namespace midend::llvm

namespace midend::llvm {  // to_string implementations

auto Function::to_string() const -> std::string {
  std::string out = "define ";
  if (is_main()) {
    out += "dso_local ";
  }

  out += get_return_type()->to_string() + " " + name + "(";
  for (size_t i = 0; i < arguments.size(); ++i) {
    out += arguments[i]->to_string();
    if (i + 1 != arguments.size()) {
      out += ", ";
    }
  }
  out += ") {\n";

  for (const auto& bb : basic_blocks) {
    out += bb->to_string();
  }
  out += "}\n\n";
  return out;
}

auto GlobalVariable::to_string() const -> std::string {
  auto ref_type =
      std::static_pointer_cast<PointerType>(type)->get_reference_type();
  return name + " = global " + ref_type->to_string() + " " +
         init_value->to_string();
}

auto Argument::to_string() const -> std::string {
  return type->to_string() + " " + name;
}

auto BasicBlock::to_string() const -> std::string {
  std::string out = name + ":\n";
  for (const auto& inst : instructions) {
    out += "  " + inst->to_string() + "\n";
  }
  return out;
}

auto ConstantString::to_string() const -> std::string {
  std::string out = "c\"";
  for (char ch : str_value) {
    if (ch == '\n') {
      out += "\\0A";
    } else if (ch == '\t') {
      out += "\\09";
    } else if (ch == '\r') {
      out += "\\0D";
    } else if (ch == '\\') {
      out += "\\\\";
    } else if (ch == '"') {
      out += "\\22";
    } else if (ch >= 32 && ch <= 126) {
      out += ch;
    } else {
      char buf[5];
      std::snprintf(buf, sizeof(buf), "\\%02X", static_cast<unsigned char>(ch));
      out += buf;
    }
  }

  if (add_null) {
    out += "\\00";
  }

  out += "\"";
  return out;
}

auto ConstantArray::to_string() const -> std::string {
  auto array_type = std::static_pointer_cast<ArrayType>(type);

  std::string out = "[";
  for (int i = 0; i < array_type->get_size(); ++i) {
    out += array_type->get_element_type()->to_string() + " ";
    if (static_cast<size_t>(i) < vals.size()) {
      out += vals[i]->to_string();
    } else {
      out += "0";
    }
    if (i + 1 != array_type->get_size()) {
      out += ", ";
    }
  }
  out += "]";
  return out;
}
}  // namespace midend::llvm

namespace midend::llvm {  // operator implementations

auto Constant::operator+(const Constant& _) const -> std::shared_ptr<Constant> {
  throw std::runtime_error(
      "constant + is not supported for this constant kind");
}

auto Constant::operator-(const Constant& _) const -> std::shared_ptr<Constant> {
  throw std::runtime_error(
      "constant - is not supported for this constant kind");
}

auto Constant::operator*(const Constant& _) const -> std::shared_ptr<Constant> {
  throw std::runtime_error(
      "constant * is not supported for this constant kind");
}

auto Constant::operator/(const Constant& _) const -> std::shared_ptr<Constant> {
  throw std::runtime_error(
      "constant / is not supported for this constant kind");
}

auto Constant::operator%(const Constant& _) const -> std::shared_ptr<Constant> {
  throw std::runtime_error(
      "constant % is not supported for this constant kind");
}

auto Constant::operator-() const -> std::shared_ptr<Constant> {
  throw std::runtime_error("unary - is not supported for this constant kind");
}

auto ConstantInt::create(int val, int bitnum) -> std::shared_ptr<ConstantInt> {
  return std::make_shared<ConstantInt>(
      IntegerType::get(bitnum), bitnum == 8 ? static_cast<char>(val) : val);
}

auto ConstantInt::operator+(const Constant& rhs) const
    -> std::shared_ptr<Constant> {
  auto rhs_int = dynamic_cast<const ConstantInt*>(&rhs);
  if (!rhs_int || type->bits_num() != rhs_int->get_type()->bits_num()) {
    throw std::runtime_error("type mismatch in constant addition");
  }
  return create(val + rhs_int->get_val(), type->bits_num());
}

auto ConstantInt::operator-(const Constant& rhs) const
    -> std::shared_ptr<Constant> {
  auto rhs_int = dynamic_cast<const ConstantInt*>(&rhs);
  if (!rhs_int || type->bits_num() != rhs_int->get_type()->bits_num()) {
    throw std::runtime_error("type mismatch in constant subtraction");
  }
  return create(val - rhs_int->get_val(), type->bits_num());
}

auto ConstantInt::operator*(const Constant& rhs) const
    -> std::shared_ptr<Constant> {
  auto rhs_int = dynamic_cast<const ConstantInt*>(&rhs);
  if (!rhs_int || type->bits_num() != rhs_int->get_type()->bits_num()) {
    throw std::runtime_error("type mismatch in constant multiplication");
  }
  return create(val * rhs_int->get_val(), type->bits_num());
}

auto ConstantInt::operator/(const Constant& rhs) const
    -> std::shared_ptr<Constant> {
  auto rhs_int = dynamic_cast<const ConstantInt*>(&rhs);
  if (!rhs_int || type->bits_num() != rhs_int->get_type()->bits_num()) {
    throw std::runtime_error("type mismatch in constant division");
  }
  assert(rhs_int->get_val() != 0 && "division by zero in constant expression");
  return create(val / rhs_int->get_val(), type->bits_num());
}

auto ConstantInt::operator%(const Constant& rhs) const
    -> std::shared_ptr<Constant> {
  auto rhs_int = dynamic_cast<const ConstantInt*>(&rhs);
  if (!rhs_int || type->bits_num() != rhs_int->get_type()->bits_num()) {
    throw std::runtime_error("type mismatch in constant modulo");
  }
  assert(rhs_int->get_val() != 0 && "modulo by zero in constant expression");
  return create(val % rhs_int->get_val(), type->bits_num());
}

auto ConstantInt::operator-() const -> std::shared_ptr<Constant> {
  return create(-val, type->bits_num());
}

auto ConstantString::create(const std::string& str, std::string name,
                            bool add_null) -> std::shared_ptr<ConstantString> {
  return std::make_shared<ConstantString>(str, std::move(name), add_null);
}

}  // namespace midend::llvm
