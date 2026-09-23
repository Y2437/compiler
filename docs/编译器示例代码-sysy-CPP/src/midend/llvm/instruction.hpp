#ifndef BUAA_COMPILER_LLVM_INSTRUCTION
#define BUAA_COMPILER_LLVM_INSTRUCTION

#include <memory>
#include <string>
#include <vector>

#include "midend/llvm/value.hpp"

namespace midend::llvm {

class Ret : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Value>& val, std::string name = "")
      -> std::shared_ptr<Ret>;
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     std::string name = "") -> std::shared_ptr<Ret>;

  auto to_string() const -> std::string override;

 private:
  Ret(const std::shared_ptr<BasicBlock>& parent_block,
      const std::shared_ptr<Type>& val_type, std::string name = "")
      : Instruction(val_type, InstructionType::RET, parent_block,
                    std::move(name)),
        is_void(false) {}

  explicit Ret(const std::shared_ptr<BasicBlock>& parent_block,
               std::string name = "")
      : Instruction(VoidType::get(), InstructionType::RET, parent_block,
                    std::move(name)),
        is_void(true) {}

 private:
  bool is_void;
};

class Br : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<BasicBlock>& target,
                     std::string name = "") -> std::shared_ptr<Br>;

  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Value>& cond,
                     const std::shared_ptr<BasicBlock>& true_target,
                     const std::shared_ptr<BasicBlock>& false_target,
                     std::string name = "") -> std::shared_ptr<Br>;

  auto is_cond_branch() const -> bool { return operands.size() == 3; }
  auto to_string() const -> std::string override;

 private:
  explicit Br(const std::shared_ptr<BasicBlock>& parent_block,
              std::string name = "")
      : Instruction(LabelType::get(), InstructionType::BR, parent_block,
                    std::move(name)) {}
};

template <Instruction::InstructionType Ty>
class BinaryInstructionImpl : public Instruction {
  static_assert(Ty >= InstructionType::ADD && Ty <= InstructionType::SREM);

 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Value>& lhs,
                     const std::shared_ptr<Value>& rhs, std::string name = "")
      -> std::shared_ptr<BinaryInstructionImpl<Ty>> {
    auto inst = std::shared_ptr<BinaryInstructionImpl<Ty>>(
        new BinaryInstructionImpl<Ty>(parent_block, lhs->get_type(),
                                      std::move(name)));
    inst->add_operand(lhs);
    inst->add_operand(rhs);
    lhs->add_user(inst);
    rhs->add_user(inst);
    return inst;
  }

  auto to_string() const -> std::string override {
    return name + " = " + BINARY_INS_TYPE_TO_STR.at(Ty) + " " +
           type->to_string() + " " + operands[0]->get_name() + ", " +
           operands[1]->get_name();
  }

 private:
  BinaryInstructionImpl(const std::shared_ptr<BasicBlock>& parent_block,
                        const std::shared_ptr<Type>& type,
                        std::string name = "")
      : Instruction(type, Ty, parent_block, std::move(name)) {}
};

using Add = BinaryInstructionImpl<Instruction::InstructionType::ADD>;
using Sub = BinaryInstructionImpl<Instruction::InstructionType::SUB>;
using Mul = BinaryInstructionImpl<Instruction::InstructionType::MUL>;
using SDiv = BinaryInstructionImpl<Instruction::InstructionType::SDIV>;
using SRem = BinaryInstructionImpl<Instruction::InstructionType::SREM>;

template <Instruction::InstructionType Ty>
class ConversionInstructionImpl : public Instruction {
  static_assert(Ty == InstructionType::TRUNC || Ty == InstructionType::ZEXT);

 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Value>& val,
                     const std::shared_ptr<Type>& target_type,
                     std::string name = "")
      -> std::shared_ptr<ConversionInstructionImpl<Ty>> {
    auto inst = std::shared_ptr<ConversionInstructionImpl<Ty>>(
        new ConversionInstructionImpl<Ty>(parent_block, target_type,
                                          std::move(name)));
    inst->add_operand(val);
    val->add_user(inst);
    return inst;
  }

  auto to_string() const -> std::string override {
    return name + " = " + CONVERSION_INS_TYPE_TO_STR.at(Ty) + " " +
           operands[0]->get_type()->to_string() + " " +
           operands[0]->get_name() + " to " + type->to_string();
  }

 private:
  ConversionInstructionImpl(const std::shared_ptr<BasicBlock>& parent_block,
                            const std::shared_ptr<Type>& type,
                            std::string name = "")
      : Instruction(type, Ty, parent_block, std::move(name)) {}
};

using Trunc = ConversionInstructionImpl<Instruction::InstructionType::TRUNC>;
using ZExt = ConversionInstructionImpl<Instruction::InstructionType::ZEXT>;

class Alloca : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Type>& content_type,
                     std::string name = "") -> std::shared_ptr<Alloca>;

  auto get_content_type() const -> std::shared_ptr<Type> {
    return std::static_pointer_cast<PointerType>(type)->get_reference_type();
  }

  auto to_string() const -> std::string override;

 private:
  Alloca(const std::shared_ptr<BasicBlock>& parent_block,
         const std::shared_ptr<Type>& content_type, std::string name = "")
      : Instruction(PointerType::get(content_type), InstructionType::ALLOCA,
                    parent_block, std::move(name)) {}
};

class Load : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Value>& addr, std::string name = "")
      -> std::shared_ptr<Load>;

  auto addr() const -> std::shared_ptr<Value> { return operands[0]; }
  auto to_string() const -> std::string override;

 private:
  Load(const std::shared_ptr<BasicBlock>& parent_block,
       const std::shared_ptr<Type>& loaded_type, std::string name = "")
      : Instruction(loaded_type, InstructionType::LOAD, parent_block,
                    std::move(name)) {}
};

class Store : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Value>& val,
                     const std::shared_ptr<Value>& addr, std::string name = "")
      -> std::shared_ptr<Store>;

  auto val() const -> std::shared_ptr<Value> { return operands[0]; }
  auto addr() const -> std::shared_ptr<Value> { return operands[1]; }

  auto to_string() const -> std::string override;

 private:
  explicit Store(const std::shared_ptr<BasicBlock>& parent_block,
                 std::string name = "")
      : Instruction(VoidType::get(), InstructionType::STORE, parent_block,
                    std::move(name)) {}
};

class ICmp : public Instruction {
 public:
  enum class ICmpType { EQ, NE, SGT, SGE, SLT, SLE, UGT, UGE, ULT, ULE };

  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     ICmpType cmp_type, const std::shared_ptr<Value>& lhs,
                     const std::shared_ptr<Value>& rhs, std::string name = "")
      -> std::shared_ptr<ICmp>;

  auto get_cmp_type() const -> ICmpType { return cmp_type; }
  auto to_string() const -> std::string override;

 private:
  ICmp(const std::shared_ptr<BasicBlock>& parent_block, ICmpType cmp_type,
       std::string name = "")
      : Instruction(IntegerType::get(1), InstructionType::ICMP, parent_block,
                    std::move(name)),
        cmp_type(cmp_type) {}

 private:
  ICmpType cmp_type;
};

class Call : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Function>& function,
                     const std::vector<std::shared_ptr<Value>>& args,
                     std::string name = "") -> std::shared_ptr<Call>;

  auto get_function() const -> std::shared_ptr<Function> {
    return std::dynamic_pointer_cast<Function>(operands[0]);
  }

  auto to_string() const -> std::string override;

 private:
  Call(const std::shared_ptr<BasicBlock>& parent_block,
       const std::shared_ptr<Type>& return_type, std::string name = "")
      : Instruction(return_type, InstructionType::CALL, parent_block,
                    std::move(name)) {}
};

class Getelementptr : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Value>& ptr,
                     const std::vector<std::shared_ptr<Value>>& indexes,
                     std::string name = "") -> std::shared_ptr<Getelementptr>;

  auto base_ptr() const -> std::shared_ptr<Value> { return operands[0]; }
  auto to_string() const -> std::string override;

 private:
  Getelementptr(const std::shared_ptr<BasicBlock>& parent_block,
                const std::shared_ptr<Type>& result_type, std::string name = "")
      : Instruction(result_type, InstructionType::GETELEMENTPTR, parent_block,
                    std::move(name)) {}
};

class Phi : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Type>& type, std::string name = "")
      -> std::shared_ptr<Phi>;

  void add_incoming(const std::shared_ptr<Value>& value,
                    const std::shared_ptr<BasicBlock>& block);
  auto get_num_incoming() const -> size_t { return operands.size() / 2; }
  auto get_incoming_value(size_t index) const -> std::shared_ptr<Value> {
    return operands[index * 2];
  }
  auto get_incoming_block(size_t index) const -> std::shared_ptr<BasicBlock> {
    return std::dynamic_pointer_cast<BasicBlock>(operands[index * 2 + 1]);
  }
  auto get_value_for_block(const std::shared_ptr<BasicBlock>& block) const
      -> std::shared_ptr<Value>;
  void set_incoming_value(size_t index, const std::shared_ptr<Value>& value) {
    operands[index * 2] = value;
  }
  void remove_incoming_for_block(const std::shared_ptr<BasicBlock>& block);
  auto to_string() const -> std::string override;

 private:
  Phi(const std::shared_ptr<BasicBlock>& parent_block,
      const std::shared_ptr<Type>& type, std::string name = "")
      : Instruction(type, InstructionType::PHI, parent_block, std::move(name)) {
  }
};

class PhiCopy : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     std::string name = "") -> std::shared_ptr<PhiCopy>;
  void add(const std::shared_ptr<Phi>& phi,
           const std::shared_ptr<Value>& value);
  void remove(const std::shared_ptr<Phi>& phi,
              const std::shared_ptr<Value>& value);
  void change_value(size_t index, const std::shared_ptr<Value>& value);
  auto get_phis() const -> const std::vector<std::shared_ptr<Phi>>& {
    return phis;
  }
  auto get_values() const -> const std::vector<std::shared_ptr<Value>>& {
    return values;
  }
  auto to_string() const -> std::string override;

 private:
  explicit PhiCopy(const std::shared_ptr<BasicBlock>& parent_block,
                   std::string name = "")
      : Instruction(VoidType::get(), InstructionType::PHICOPY, parent_block,
                    std::move(name)) {}

  std::vector<std::shared_ptr<Phi>> phis;
  std::vector<std::shared_ptr<Value>> values;
};

class Move : public Instruction {
 public:
  static auto create(const std::shared_ptr<BasicBlock>& parent_block,
                     const std::shared_ptr<Value>& src,
                     const std::shared_ptr<Value>& dst, std::string name = "")
      -> std::shared_ptr<Move>;
  auto src() const -> std::shared_ptr<Value> { return operands[0]; }
  auto dst() const -> std::shared_ptr<Value> { return operands[1]; }
  auto to_string() const -> std::string override;

 private:
  Move(const std::shared_ptr<BasicBlock>& parent_block,
       const std::shared_ptr<Type>& type, std::string name = "")
      : Instruction(type, InstructionType::MOVE, parent_block,
                    std::move(name)) {}
};

}  // namespace midend::llvm

#endif  // BUAA_COMPILER_LLVM_INSTRUCTION
