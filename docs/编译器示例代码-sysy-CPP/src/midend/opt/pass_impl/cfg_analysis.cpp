#include <queue>
#include <unordered_set>

#include "midend/llvm/instruction.hpp"
#include "midend/opt/opt_util.hpp"
#include "midend/opt/pass.hpp"
#include "midend/opt/support.hpp"

namespace midend::opt::pass {

static void clear_cfg_info(const std::shared_ptr<llvm::Function>& func) {
  for (const auto& block : func->get_basic_blocks()) {
    block->opt_info.predecessors.clear();
    block->opt_info.successors.clear();
  }
}

static std::vector<std::shared_ptr<llvm::BasicBlock>> get_branch_targets(
    const std::shared_ptr<llvm::BasicBlock>& block) {
  std::vector<std::shared_ptr<llvm::BasicBlock>> targets;

  const auto& instructions = block->get_instructions();
  if (instructions.empty()) {
    return targets;
  }

  auto last_inst = instructions.back();

  if (auto br = std::dynamic_pointer_cast<llvm::Br>(last_inst)) {
    const auto& operands = br->get_operands();

    if (br->is_cond_branch()) {
      if (operands.size() >= 3) {
        if (auto true_target =
                std::dynamic_pointer_cast<llvm::BasicBlock>(operands[1])) {
          targets.push_back(true_target);
        }
        if (auto false_target =
                std::dynamic_pointer_cast<llvm::BasicBlock>(operands[2])) {
          targets.push_back(false_target);
        }
      }
    } else {
      if (!operands.empty()) {
        if (auto target =
                std::dynamic_pointer_cast<llvm::BasicBlock>(operands[0])) {
          targets.push_back(target);
        }
      }
    }
  }

  return targets;
}

static void build_cfg_for_function(
    const std::shared_ptr<llvm::Function>& func) {
  clear_cfg_info(func);

  for (const auto& block : func->get_basic_blocks()) {
    auto targets = get_branch_targets(block);

    for (const auto& target : targets) {
      block->opt_info.successors.push_back(target);
      target->opt_info.predecessors.push_back(block);
    }
  }
}

bool ControlFlowGraphAnalysis::run(llvm::Module& module) {
  for (const auto& func : module.get_functions()) {
    build_cfg_for_function(func);
  }

  return false;
}

bool RemoveUnreachableInstructions::run(llvm::Module& module) {
  bool modified = false;

  for (const auto& func : module.get_functions()) {
    for (const auto& block : func->get_basic_blocks()) {
      auto& instructions = block->get_instructions_ref();
      bool found_terminator = false;

      for (auto it = instructions.begin(); it != instructions.end();) {
        if (found_terminator) {
          util::remove_all_operands(*it);
          it = instructions.erase(it);
          modified = true;
        } else {
          if (util::is_terminator(*it)) {
            found_terminator = true;
          }
          ++it;
        }
      }
    }
  }

  return modified;
}

static void remove_phi_incoming_from_block(
    const std::shared_ptr<llvm::BasicBlock>& block,
    const std::shared_ptr<llvm::BasicBlock>& removed_block) {
  for (const auto& inst : block->get_instructions()) {
    if (inst->get_instruction_type() !=
        llvm::Instruction::InstructionType::PHI) {
      break;
    }
    auto phi = std::dynamic_pointer_cast<llvm::Phi>(inst);
    if (phi) {
      phi->remove_incoming_for_block(removed_block);
    }
  }
}

bool RemoveUnreachableBlocks::run(llvm::Module& module) {
  bool modified = false;

  for (const auto& func : module.get_functions()) {
    const auto& blocks = func->get_basic_blocks();
    if (blocks.empty()) continue;

    std::unordered_set<std::shared_ptr<llvm::BasicBlock>> reachable;
    std::queue<std::shared_ptr<llvm::BasicBlock>> worklist;

    auto entry = blocks.front();
    worklist.push(entry);
    reachable.insert(entry);

    while (!worklist.empty()) {
      auto current = worklist.front();
      worklist.pop();

      for (const auto& weak_succ : current->opt_info.successors) {
        if (auto succ = weak_succ.lock()) {
          if (reachable.find(succ) == reachable.end()) {
            reachable.insert(succ);
            worklist.push(succ);
          }
        }
      }
    }

    std::vector<std::shared_ptr<llvm::BasicBlock>> unreachable_blocks;
    for (const auto& block : blocks) {
      if (reachable.find(block) == reachable.end()) {
        unreachable_blocks.push_back(block);
      }
    }

    for (const auto& block : unreachable_blocks) {
      for (const auto& weak_succ : block->opt_info.successors) {
        if (auto succ = weak_succ.lock()) {
          remove_phi_incoming_from_block(succ, block);
          auto& preds = succ->opt_info.predecessors;
          preds.erase(std::remove_if(
                          preds.begin(), preds.end(),
                          [&block](const std::weak_ptr<llvm::BasicBlock>& p) {
                            return p.lock() == block;
                          }),
                      preds.end());
        }
      }
    }

    auto& blocks_ref = func->get_basic_blocks_ref();
    for (auto it = blocks_ref.begin(); it != blocks_ref.end();) {
      if (reachable.find(*it) == reachable.end()) {
        it = blocks_ref.erase(it);
        modified = true;
      } else {
        ++it;
      }
    }
  }

  return modified;
}

}  // namespace midend::opt::pass
