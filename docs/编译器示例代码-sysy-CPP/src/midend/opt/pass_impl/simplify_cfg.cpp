#include <algorithm>
#include <unordered_set>

#include "midend/llvm/instruction.hpp"
#include "midend/opt/opt_util.hpp"
#include "midend/opt/pass.hpp"

namespace midend::opt::pass {

static void update_phi_predecessor(
    const std::shared_ptr<llvm::BasicBlock>& succ,
    const std::shared_ptr<llvm::BasicBlock>& old_block,
    const std::shared_ptr<llvm::BasicBlock>& new_block) {
  for (const auto& inst : succ->get_instructions()) {
    if (inst->get_instruction_type() !=
        llvm::Instruction::InstructionType::PHI) {
      break;
    }
    auto phi = std::dynamic_pointer_cast<llvm::Phi>(inst);
    if (!phi) continue;

    for (size_t i = 0; i < phi->get_num_incoming(); ++i) {
      if (phi->get_incoming_block(i) == old_block) {
        // A PHI stores alternating value and predecessor operands.
        phi->set_operand(i * 2 + 1, new_block);
        old_block->remove_user(phi);
        new_block->add_user(phi);
      }
    }
  }
}

static void remove_phi_predecessor(
    const std::shared_ptr<llvm::BasicBlock>& succ,
    const std::shared_ptr<llvm::BasicBlock>& pred) {
  for (const auto& inst : succ->get_instructions()) {
    if (inst->get_instruction_type() !=
        llvm::Instruction::InstructionType::PHI) {
      break;
    }
    auto phi = std::dynamic_pointer_cast<llvm::Phi>(inst);
    if (phi) {
      phi->remove_incoming_for_block(pred);
    }
  }
}

bool BlockMerge::run(llvm::Module& module) {
  bool modified = false;

  for (const auto& func : module.get_functions()) {
    auto& blocks = func->get_basic_blocks_ref();

    bool changed = true;
    while (changed) {
      changed = false;

      for (auto it = blocks.begin(); it != blocks.end();) {
        auto block = *it;

        if (block == func->entry_block()) {
          ++it;
          continue;
        }

        const auto& preds = block->opt_info.predecessors;
        if (preds.size() != 1) {
          ++it;
          continue;
        }

        auto pred = preds[0].lock();
        if (!pred) {
          ++it;
          continue;
        }

        const auto& pred_succs = pred->opt_info.successors;
        if (pred_succs.size() != 1) {
          ++it;
          continue;
        }

        auto terminator = pred->get_terminator();
        if (!terminator) {
          ++it;
          continue;
        }

        auto br = std::dynamic_pointer_cast<llvm::Br>(terminator);
        if (!br || br->is_cond_branch()) {
          ++it;
          continue;
        }

        bool has_phi = false;
        for (const auto& inst : block->get_instructions()) {
          if (inst->get_instruction_type() ==
              llvm::Instruction::InstructionType::PHI) {
            auto phi = std::dynamic_pointer_cast<llvm::Phi>(inst);
            if (phi && phi->get_num_incoming() == 1) {
              auto value = phi->get_incoming_value(0);
              util::replace_all_uses_with(phi, value);
            } else {
              has_phi = true;
              break;
            }
          } else {
            break;
          }
        }

        if (has_phi) {
          ++it;
          continue;
        }

        auto& block_insts = block->get_instructions_ref();
        for (auto inst_it = block_insts.begin();
             inst_it != block_insts.end();) {
          if ((*inst_it)->get_instruction_type() ==
              llvm::Instruction::InstructionType::PHI) {
            auto phi = *inst_it;
            util::remove_all_operands(phi);
            inst_it = block_insts.erase(inst_it);
          } else {
            break;
          }
        }

        auto& pred_insts = pred->get_instructions_ref();
        util::remove_all_operands(br);
        pred_insts.pop_back();

        for (const auto& inst : block_insts) {
          inst->set_parent_block(pred);
        }
        pred_insts.splice(pred_insts.end(), block_insts);

        pred->opt_info.successors = block->opt_info.successors;

        for (const auto& weak_succ : pred->opt_info.successors) {
          if (auto succ = weak_succ.lock()) {
            update_phi_predecessor(succ, block, pred);

            for (auto& weak_pred : succ->opt_info.predecessors) {
              if (weak_pred.lock() == block) {
                weak_pred = pred;
              }
            }
          }
        }

        it = blocks.erase(it);
        changed = true;
        modified = true;
      }
    }
  }

  return modified;
}

bool SimplifyCFG::run(llvm::Module& module) {
  bool modified = false;

  for (const auto& func : module.get_functions()) {
    for (const auto& block : func->get_basic_blocks()) {
      auto terminator = block->get_terminator();
      if (!terminator) continue;

      auto br = std::dynamic_pointer_cast<llvm::Br>(terminator);
      if (!br || !br->is_cond_branch()) continue;

      auto cond = br->get_operand(0);
      auto const_cond = std::dynamic_pointer_cast<llvm::ConstantInt>(cond);
      if (!const_cond) continue;

      auto true_target =
          std::dynamic_pointer_cast<llvm::BasicBlock>(br->get_operand(1));
      auto false_target =
          std::dynamic_pointer_cast<llvm::BasicBlock>(br->get_operand(2));

      if (!true_target || !false_target) continue;

      auto target = const_cond->get_val() ? true_target : false_target;
      auto dead_target = const_cond->get_val() ? false_target : true_target;

      util::remove_all_operands(br);

      auto& instructions = block->get_instructions_ref();
      instructions.pop_back();

      auto new_br = llvm::Br::create(block, target);
      instructions.push_back(new_br);

      block->opt_info.successors.clear();
      block->opt_info.successors.push_back(target);

      remove_phi_predecessor(dead_target, block);
      dead_target->opt_info.remove_predecessor(block);

      modified = true;
    }

    auto& blocks = func->get_basic_blocks_ref();
    std::unordered_set<std::shared_ptr<llvm::BasicBlock>> to_remove;

    for (const auto& block : blocks) {
      if (block == func->entry_block()) continue;

      const auto& instructions = block->get_instructions();

      if (instructions.size() != 1) continue;

      auto br = std::dynamic_pointer_cast<llvm::Br>(instructions.front());
      if (!br || br->is_cond_branch()) continue;

      auto target =
          std::dynamic_pointer_cast<llvm::BasicBlock>(br->get_operand(0));
      if (!target || target == block) continue;

      const auto& target_instructions = target->get_instructions();
      bool target_has_phi =
          !target_instructions.empty() &&
          target_instructions.front()->get_instruction_type() ==
              llvm::Instruction::InstructionType::PHI;
      // Redirecting this edge would require duplicating PHI inputs.
      if (target_has_phi) continue;

      if (block->opt_info.predecessors.empty()) continue;

      for (const auto& weak_pred : block->opt_info.predecessors) {
        auto pred = weak_pred.lock();
        if (!pred) continue;

        auto pred_term = pred->get_terminator();
        if (!pred_term) continue;

        util::substitute_operand(pred_term, block, target);

        for (auto& weak_succ : pred->opt_info.successors) {
          if (weak_succ.lock() == block) {
            weak_succ = target;
          }
        }
      }

      for (const auto& weak_pred : block->opt_info.predecessors) {
        auto pred = weak_pred.lock();
        if (!pred) continue;

        const auto& target_predecessors = target->opt_info.predecessors;
        if (std::none_of(
                target_predecessors.begin(), target_predecessors.end(),
                [&pred](const auto& item) { return item.lock() == pred; })) {
          target->opt_info.predecessors.push_back(pred);
        }
      }

      target->opt_info.remove_predecessor(block);

      to_remove.insert(block);
      modified = true;
    }

    for (auto it = blocks.begin(); it != blocks.end();) {
      if (to_remove.count(*it)) {
        for (const auto& inst : (*it)->get_instructions()) {
          util::remove_all_operands(inst);
        }
        it = blocks.erase(it);
      } else {
        ++it;
      }
    }
  }

  return modified;
}

}  // namespace midend::opt::pass
