#include <memory>
#include <vector>

#include "midend/llvm/instruction.hpp"
#include "midend/opt/opt_util.hpp"
#include "midend/opt/pass.hpp"

namespace midend::opt::pass {

static auto try_fold_binary_constant(
    llvm::Instruction::InstructionType ins_type,
    const std::shared_ptr<llvm::Value>& left,
    const std::shared_ptr<llvm::Value>& right)
    -> std::shared_ptr<llvm::Constant> {
  using IT = llvm::Instruction::InstructionType;

  auto left_const = std::dynamic_pointer_cast<llvm::ConstantInt>(left);
  auto right_const = std::dynamic_pointer_cast<llvm::ConstantInt>(right);

  if (!left_const || !right_const) {
    return nullptr;
  }

  int lval = left_const->get_val();
  int rval = right_const->get_val();
  int result = 0;

  switch (ins_type) {
    case IT::ADD:
      result = lval + rval;
      break;
    case IT::SUB:
      result = lval - rval;
      break;
    case IT::MUL:
      result = lval * rval;
      break;
    case IT::SDIV:
      if (rval == 0) return nullptr;
      result = lval / rval;
      break;
    case IT::SREM:
      if (rval == 0) return nullptr;
      result = lval % rval;
      break;
    case IT::AND:
      result = lval & rval;
      break;
    case IT::OR:
      result = lval | rval;
      break;
    case IT::XOR:
      result = lval ^ rval;
      break;
    case IT::SHL:
      result = lval << rval;
      break;
    case IT::LSHR:
      result = static_cast<unsigned>(lval) >> rval;
      break;
    case IT::ASHR:
      result = lval >> rval;
      break;
    default:
      return nullptr;
  }

  return llvm::ConstantInt::create(result, left->get_type()->bits_num());
}

static auto try_fold_icmp(llvm::ICmp::ICmpType cmp_type,
                          const std::shared_ptr<llvm::Value>& left,
                          const std::shared_ptr<llvm::Value>& right)
    -> std::shared_ptr<llvm::Constant> {
  auto left_const = std::dynamic_pointer_cast<llvm::ConstantInt>(left);
  auto right_const = std::dynamic_pointer_cast<llvm::ConstantInt>(right);

  if (!left_const || !right_const) {
    return nullptr;
  }

  int lval = left_const->get_val();
  int rval = right_const->get_val();
  bool result = false;

  using CT = llvm::ICmp::ICmpType;
  switch (cmp_type) {
    case CT::EQ:
      result = (lval == rval);
      break;
    case CT::NE:
      result = (lval != rval);
      break;
    case CT::SGT:
      result = (lval > rval);
      break;
    case CT::SGE:
      result = (lval >= rval);
      break;
    case CT::SLT:
      result = (lval < rval);
      break;
    case CT::SLE:
      result = (lval <= rval);
      break;
    case CT::UGT:
      result = (static_cast<unsigned>(lval) > static_cast<unsigned>(rval));
      break;
    case CT::UGE:
      result = (static_cast<unsigned>(lval) >= static_cast<unsigned>(rval));
      break;
    case CT::ULT:
      result = (static_cast<unsigned>(lval) < static_cast<unsigned>(rval));
      break;
    case CT::ULE:
      result = (static_cast<unsigned>(lval) <= static_cast<unsigned>(rval));
      break;
  }

  return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(1),
                                             result ? 1 : 0);
}

static std::shared_ptr<llvm::Value> simplify_binary_inst(
    const std::shared_ptr<llvm::Instruction>& inst) {
  using IT = llvm::Instruction::InstructionType;

  auto ins_type = inst->get_instruction_type();
  const auto& operands = inst->get_operands();

  if (operands.size() < 2) return nullptr;

  auto left = operands[0];
  auto right = operands[1];

  if (auto folded = try_fold_binary_constant(ins_type, left, right)) {
    return folded;
  }

  auto left_const = std::dynamic_pointer_cast<llvm::ConstantInt>(left);
  auto right_const = std::dynamic_pointer_cast<llvm::ConstantInt>(right);

  switch (ins_type) {
    case IT::ADD:
      // x + 0 = x
      if (right_const && right_const->get_val() == 0) return left;
      // 0 + x = x
      if (left_const && left_const->get_val() == 0) return right;
      break;

    case IT::SUB:
      // x - 0 = x
      if (right_const && right_const->get_val() == 0) return left;
      // x - x = 0
      if (left == right) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      break;

    case IT::MUL:
      // x * 0 = 0
      if (right_const && right_const->get_val() == 0) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      if (left_const && left_const->get_val() == 0) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      // x * 1 = x
      if (right_const && right_const->get_val() == 1) return left;
      if (left_const && left_const->get_val() == 1) return right;
      break;

    case IT::SDIV:
      // x / 1 = x
      if (right_const && right_const->get_val() == 1) return left;
      // 0 / x = 0 (x != 0)
      if (left_const && left_const->get_val() == 0) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      // x / x = 1 (x != 0)
      if (left == right) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   1);
      }
      break;

    case IT::SREM:
      // x % 1 = 0
      if (right_const && right_const->get_val() == 1) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      // 0 % x = 0
      if (left_const && left_const->get_val() == 0) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      // x % x = 0
      if (left == right) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      break;

    case IT::AND:
      // x & 0 = 0
      if (right_const && right_const->get_val() == 0) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      // x & -1 = x
      if (right_const && right_const->get_val() == -1) return left;
      // x & x = x
      if (left == right) return left;
      break;

    case IT::OR:
      // x | 0 = x
      if (right_const && right_const->get_val() == 0) return left;
      // x | -1 = -1
      if (right_const && right_const->get_val() == -1) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   -1);
      }
      // x | x = x
      if (left == right) return left;
      break;

    case IT::XOR:
      // x ^ 0 = x
      if (right_const && right_const->get_val() == 0) return left;
      // x ^ x = 0
      if (left == right) {
        return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(32),
                                                   0);
      }
      break;

    case IT::SHL:
    case IT::LSHR:
    case IT::ASHR:
      // x << 0 = x, x >> 0 = x
      if (right_const && right_const->get_val() == 0) return left;
      break;

    default:
      break;
  }

  return nullptr;
}

static std::shared_ptr<llvm::Value> simplify_icmp_inst(
    const std::shared_ptr<llvm::ICmp>& icmp) {
  const auto& operands = icmp->get_operands();
  if (operands.size() < 2) return nullptr;

  auto left = operands[0];
  auto right = operands[1];
  auto cmp_type = icmp->get_cmp_type();

  if (auto folded = try_fold_icmp(cmp_type, left, right)) {
    return folded;
  }

  if (left == right) {
    using CT = llvm::ICmp::ICmpType;
    bool result = false;
    switch (cmp_type) {
      case CT::EQ:
      case CT::SGE:
      case CT::SLE:
      case CT::UGE:
      case CT::ULE:
        result = true;
        break;
      case CT::NE:
      case CT::SGT:
      case CT::SLT:
      case CT::UGT:
      case CT::ULT:
        result = false;
        break;
    }
    return std::make_shared<llvm::ConstantInt>(llvm::IntegerType::get(1),
                                               result ? 1 : 0);
  }

  return nullptr;
}

static std::shared_ptr<llvm::Value> simplify_conversion_inst(
    const std::shared_ptr<llvm::Instruction>& inst) {
  using IT = llvm::Instruction::InstructionType;

  const auto& operands = inst->get_operands();
  if (operands.empty()) return nullptr;

  auto src = operands[0];
  auto src_const = std::dynamic_pointer_cast<llvm::ConstantInt>(src);

  if (!src_const) return nullptr;

  int val = src_const->get_val();

  switch (inst->get_instruction_type()) {
    case IT::ZEXT: {
      const int source_bits = src->get_type()->bits_num();
      const int mask = source_bits >= 32 ? -1 : (1 << source_bits) - 1;
      return llvm::ConstantInt::create(val & mask,
                                       inst->get_type()->bits_num());
    }

    case IT::TRUNC:
      if (inst->get_type()->is_integer_ty()) {
        auto target_type =
            std::dynamic_pointer_cast<llvm::IntegerType>(inst->get_type());
        if (target_type) {
          const int bits = target_type->bits_num();
          const int mask = bits >= 32 ? -1 : (1 << bits) - 1;
          return llvm::ConstantInt::create(val & mask, bits);
        }
      }
      break;

    default:
      break;
  }

  return nullptr;
}

bool ConstantFolding::run(llvm::Module& module) {
  bool modified = false;

  for (const auto& func : module.get_functions()) {
    for (const auto& block : func->get_basic_blocks()) {
      auto& instructions = block->get_instructions_ref();

      for (auto it = instructions.begin(); it != instructions.end();) {
        auto inst = *it;
        std::shared_ptr<llvm::Value> simplified = nullptr;

        auto ins_type = inst->get_instruction_type();
        using IT = llvm::Instruction::InstructionType;

        if (ins_type >= IT::ADD && ins_type <= IT::ASHR) {
          simplified = simplify_binary_inst(inst);
        } else if (ins_type == IT::ICMP) {
          if (auto icmp = std::dynamic_pointer_cast<llvm::ICmp>(inst)) {
            simplified = simplify_icmp_inst(icmp);
          }
        } else if (ins_type == IT::ZEXT || ins_type == IT::TRUNC) {
          simplified = simplify_conversion_inst(inst);
        }

        if (simplified) {
          util::replace_all_uses_with(inst, simplified);
          util::remove_all_operands(inst);
          it = instructions.erase(it);
          modified = true;
        } else {
          ++it;
        }
      }
    }
  }

  return modified;
}

}  // namespace midend::opt::pass
