#include <unordered_set>
#include <vector>

#include "midend/llvm/instruction.hpp"
#include "midend/opt/opt_util.hpp"
#include "midend/opt/pass.hpp"

namespace midend::opt::pass {

static bool has_side_effect(const std::shared_ptr<llvm::Instruction>& inst) {
  using IT = llvm::Instruction::InstructionType;
  auto ins_type = inst->get_instruction_type();

  if (ins_type == IT::RET || ins_type == IT::BR || ins_type == IT::STORE) {
    return true;
  }

  // Without interprocedural analysis, every call must be kept.
  return ins_type == IT::CALL;
}

static void mark_useful(
    const std::shared_ptr<llvm::Instruction>& inst,
    std::unordered_set<std::shared_ptr<llvm::Instruction>>& useful_set) {
  if (useful_set.find(inst) != useful_set.end()) {
    return;
  }

  useful_set.insert(inst);

  for (const auto& operand : inst->get_operands()) {
    if (auto op_inst = std::dynamic_pointer_cast<llvm::Instruction>(operand)) {
      mark_useful(op_inst, useful_set);
    }
  }
}

bool DeadCodeElimination::run(llvm::Module& module) {
  bool modified = false;

  for (const auto& func : module.get_functions()) {
    std::unordered_set<std::shared_ptr<llvm::Instruction>> useful_instructions;

    for (const auto& block : func->get_basic_blocks()) {
      for (const auto& inst : block->get_instructions()) {
        if (has_side_effect(inst)) {
          mark_useful(inst, useful_instructions);
        }
      }
    }

    std::vector<std::shared_ptr<llvm::Instruction>> dead_instructions;
    for (const auto& block : func->get_basic_blocks()) {
      for (const auto& inst : block->get_instructions()) {
        if (useful_instructions.find(inst) == useful_instructions.end()) {
          dead_instructions.push_back(inst);
        }
      }
    }

    for (const auto& dead_inst : dead_instructions) {
      util::remove_instruction(dead_inst);
      modified = true;
    }
  }

  return modified;
}

}  // namespace midend::opt::pass
