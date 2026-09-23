#include "midend/llvm/instruction.hpp"
#include "midend/opt/opt_util.hpp"
#include "midend/opt/pass.hpp"

namespace midend::opt::pass {

namespace {

bool remove_phi_for_function(const std::shared_ptr<llvm::Function>& function);
void insert_parallel_copy(const std::shared_ptr<llvm::BasicBlock>& predecessor,
                          const std::shared_ptr<llvm::BasicBlock>& successor);
bool remove_parallel_copy(const std::shared_ptr<llvm::Function>& function);

void insert_before_in_block(const std::shared_ptr<llvm::BasicBlock>& block,
                            const std::shared_ptr<llvm::Instruction>& inst,
                            const std::shared_ptr<llvm::Instruction>& before) {
  auto& instructions = block->get_instructions_ref();
  for (auto it = instructions.begin(); it != instructions.end(); ++it) {
    if (*it == before) {
      inst->set_parent_block(block);
      instructions.insert(it, inst);
      return;
    }
  }
}

bool remove_phi_for_function(const std::shared_ptr<llvm::Function>& function) {
  bool modified = false;
  for (auto& basic_block : function->get_basic_blocks()) {
    if (basic_block->get_instructions_ref().empty()) {
      continue;
    }
    auto first_inst = basic_block->get_instructions_ref().front();
    if (first_inst->get_instruction_type() !=
        llvm::Instruction::InstructionType::PHI) {
      continue;
    }

    modified = true;
    // Splitting a critical edge can change the predecessor list.
    auto predecessors_copy = basic_block->opt_info.predecessors;

    for (auto& predecessor_weak : predecessors_copy) {
      auto predecessor = predecessor_weak.lock();
      if (!predecessor) continue;
      insert_parallel_copy(predecessor, basic_block);
    }

    std::vector<std::shared_ptr<llvm::Instruction>> to_remove;
    for (auto& instruction : basic_block->get_instructions()) {
      auto phi = std::dynamic_pointer_cast<llvm::Phi>(instruction);
      if (!phi) break;

      for (auto& predecessor_weak : basic_block->opt_info.predecessors) {
        auto predecessor = predecessor_weak.lock();
        if (!predecessor) continue;

        auto value = util::get_phi_value(phi, predecessor);
        // A split edge keeps the original predecessor in the PHI node.
        if (!value) {
          if (!predecessor->opt_info.predecessors.empty()) {
            auto orig_pred = predecessor->opt_info.predecessors[0].lock();
            if (orig_pred) {
              value = util::get_phi_value(phi, orig_pred);
            }
          }
        }
        if (!value) continue;

        auto terminator = predecessor->get_terminator();
        if (!terminator) continue;

        auto& instructions = predecessor->get_instructions_ref();
        for (auto it = instructions.begin(); it != instructions.end(); ++it) {
          if (*it == terminator && it != instructions.begin()) {
            auto prev_it = std::prev(it);
            if (auto phi_copy =
                    std::dynamic_pointer_cast<llvm::PhiCopy>(*prev_it)) {
              phi_copy->add(phi, value);
            }
            break;
          }
        }
      }

      to_remove.push_back(instruction);
    }

    for (auto& inst : to_remove) {
      util::remove_all_operands(inst);
      util::remove_instruction_from_parent(inst);
    }
  }
  return modified;
}

void insert_parallel_copy(const std::shared_ptr<llvm::BasicBlock>& predecessor,
                          const std::shared_ptr<llvm::BasicBlock>& successor) {
  auto phi_copy = llvm::PhiCopy::create(nullptr, util::gen_temp_name());

  if (predecessor->opt_info.successors.size() == 1) {
    auto terminator = predecessor->get_terminator();
    if (terminator) {
      insert_before_in_block(predecessor, phi_copy, terminator);
    }
    return;
  }

  // Put the parallel copy in a bridge block when the edge is critical.
  auto parent_func = successor->get_parent_func().lock();
  if (!parent_func) return;

  auto new_block =
      std::make_shared<llvm::BasicBlock>(parent_func, util::gen_block_name());
  phi_copy->set_parent_block(new_block);

  auto br = llvm::Br::create(new_block, successor, util::gen_temp_name());
  new_block->add_instructions({phi_copy, br});

  parent_func->get_basic_blocks_ref().push_back(new_block);

  auto pred_terminator = predecessor->get_terminator();
  if (pred_terminator) {
    util::substitute_operand(pred_terminator, successor, new_block);
  }

  predecessor->opt_info.successors.push_back(new_block);
  predecessor->opt_info.remove_successor(successor);
  new_block->opt_info.predecessors.push_back(predecessor);
  new_block->opt_info.successors.push_back(successor);
  successor->opt_info.predecessors.push_back(new_block);
  successor->opt_info.remove_predecessor(predecessor);
}

bool remove_parallel_copy(const std::shared_ptr<llvm::Function>& function) {
  bool modified = false;
  for (auto& basic_block : function->get_basic_blocks_ref()) {
    auto terminator = basic_block->get_terminator();
    if (!terminator) continue;

    std::vector<std::shared_ptr<llvm::Move>> moves;
    std::vector<std::shared_ptr<llvm::PhiCopy>> phi_copies_to_remove;

    auto& instructions = basic_block->get_instructions_ref();
    for (auto it = instructions.begin(); it != instructions.end(); ++it) {
      if (*it == terminator) break;

      auto phi_copy = std::dynamic_pointer_cast<llvm::PhiCopy>(*it);
      if (!phi_copy) continue;

      modified = true;
      phi_copies_to_remove.push_back(phi_copy);

      while (!phi_copy->get_phis().empty()) {
        auto phis = phi_copy->get_phis();
        auto values = phi_copy->get_values();
        bool made_progress = false;

        for (size_t i = 0; i < values.size(); ++i) {
          const auto& phi = phis[i];
          const auto& value = values[i];

          bool has_dependency = false;
          for (const auto& v : values) {
            if (v == phi && v != value) {
              has_dependency = true;
              break;
            }
          }

          if (phi == value) {
            phi_copy->remove(phi, value);
            made_progress = true;
            break;
          }

          if (!has_dependency) {
            auto move = llvm::Move::create(basic_block, value, phi, "");
            moves.push_back(move);
            phi_copy->remove(phi, value);
            made_progress = true;
            break;
          }
        }

        if (!made_progress && !phi_copy->get_phis().empty()) {
          // Break a copy cycle such as a <- b, b <- a with a temporary.
          auto first_value = phi_copy->get_values().front();
          auto placeholder = std::make_shared<llvm::Placeholder>(
              first_value->get_type(), "placeholder_" + util::gen_temp_name());

          auto move =
              llvm::Move::create(basic_block, first_value, placeholder, "");
          moves.push_back(move);

          phi_copy->change_value(0, placeholder);
        }
      }
    }

    for (const auto& pc : phi_copies_to_remove) {
      util::remove_instruction(pc);
    }

    for (const auto& move : moves) {
      insert_before_in_block(basic_block, move, terminator);
    }
  }
  return modified;
}

}  // anonymous namespace

bool RemovePhi::run(llvm::Module& module) {
  bool modified = false;
  for (const auto& function : module.get_functions()) {
    modified |= remove_phi_for_function(function);
    modified |= remove_parallel_copy(function);
  }

  return modified;
}

}  // namespace midend::opt::pass
