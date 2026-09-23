#include "midend/opt/opt_util.hpp"

#include "midend/llvm/instruction.hpp"
#include "midend/llvm/value.hpp"

namespace midend::opt::util {

void replace_all_uses_with(const std::shared_ptr<llvm::Value>& old_value,
                           const std::shared_ptr<llvm::Value>& new_value) {
  if (!old_value || old_value == new_value) {
    return;
  }

  // Replacing operands mutates the use list, so iterate over a copy.
  auto users = old_value->get_users();

  for (auto& weak_user : users) {
    if (auto user = weak_user.lock()) {
      auto& operands = user->get_operands();
      for (size_t i = 0; i < operands.size(); ++i) {
        if (operands[i] == old_value) {
          old_value->remove_user(user);
          operands[i] = new_value;
          if (new_value) {
            new_value->add_user(user);
          }
        }
      }
    }
  }

  old_value->get_users().clear();
}

void remove_all_operands(const std::shared_ptr<llvm::User>& user) {
  for (auto& operand : user->get_operands()) {
    if (operand) {
      operand->remove_user(user);
    }
  }
  user->get_operands().clear();
}

void remove_instruction(const std::shared_ptr<llvm::Instruction>& inst) {
  remove_all_operands(inst);
  if (auto parent = inst->get_parent_block().lock()) {
    auto& instructions = parent->get_instructions_ref();
    instructions.remove(inst);
  }

  inst->get_users().clear();
}

void remove_instruction_from_parent(
    const std::shared_ptr<llvm::Instruction>& inst) {
  if (auto parent = inst->get_parent_block().lock()) {
    auto& instructions = parent->get_instructions_ref();
    instructions.remove(inst);
  }
}

void substitute_operand(const std::shared_ptr<llvm::User>& user,
                        const std::shared_ptr<llvm::Value>& old_operand,
                        const std::shared_ptr<llvm::Value>& new_operand) {
  auto& operands = user->get_operands();
  for (size_t i = 0; i < operands.size(); ++i) {
    if (operands[i] == old_operand) {
      old_operand->remove_user(user);
      operands[i] = new_operand;
      new_operand->add_user(user);
    }
  }
}

auto get_phi_value(const std::shared_ptr<llvm::Phi>& phi,
                   const std::shared_ptr<llvm::BasicBlock>& block)
    -> std::shared_ptr<llvm::Value> {
  for (size_t i = 0; i < phi->get_num_incoming(); ++i) {
    if (phi->get_incoming_block(i) == block) {
      return phi->get_incoming_value(i);
    }
  }
  return nullptr;
}

bool is_terminator(const std::shared_ptr<llvm::Instruction>& instruction) {
  auto ins_type = instruction->get_instruction_type();
  using IT = llvm::Instruction::InstructionType;

  return ins_type == IT::RET || ins_type == IT::BR;
}

}  // namespace midend::opt::util
