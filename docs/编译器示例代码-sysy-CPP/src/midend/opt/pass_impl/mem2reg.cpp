#include <deque>
#include <unordered_map>
#include <unordered_set>

#include "midend/llvm/instruction.hpp"
#include "midend/opt/opt_util.hpp"
#include "midend/opt/pass.hpp"

namespace midend::opt::pass {

using BlockPtr = std::shared_ptr<llvm::BasicBlock>;
using AllocaPtr = std::shared_ptr<llvm::Alloca>;
using ValuePtr = std::shared_ptr<llvm::Value>;
using PhiPtr = std::shared_ptr<llvm::Phi>;

static int phi_counter = 0;

static std::string gen_phi_name() {
  return "%phi_" + std::to_string(phi_counter++);
}

static bool is_promotable(const AllocaPtr& alloca) {
  auto ptr_type =
      std::dynamic_pointer_cast<llvm::PointerType>(alloca->get_type());
  if (!ptr_type) return false;

  auto content_type = ptr_type->get_reference_type();

  if (!content_type->is_integer_ty()) {
    return false;
  }

  for (const auto& weak_user : alloca->get_users()) {
    auto user = weak_user.lock();
    if (!user) continue;

    auto inst = std::dynamic_pointer_cast<llvm::Instruction>(user);
    if (!inst) return false;

    auto ins_type = inst->get_instruction_type();
    if (ins_type != llvm::Instruction::InstructionType::LOAD &&
        ins_type != llvm::Instruction::InstructionType::STORE) {
      return false;
    }

    if (ins_type == llvm::Instruction::InstructionType::STORE) {
      auto store = std::dynamic_pointer_cast<llvm::Store>(inst);
      if (store->val() == alloca) {
        return false;
      }
    }
  }

  return true;
}

static void update_phi_incoming(const PhiPtr& phi, const BlockPtr& block,
                                const ValuePtr& new_value) {
  for (size_t i = 0; i < phi->get_num_incoming(); ++i) {
    if (phi->get_incoming_block(i) == block) {
      auto old_value = phi->get_incoming_value(i);
      if (old_value) {
        old_value->remove_user(phi);
      }
      phi->set_incoming_value(i, new_value);
      if (new_value) {
        new_value->add_user(phi);
      }
      return;
    }
  }
}

class Mem2RegImpl {
 public:
  bool run(const std::shared_ptr<llvm::Function>& func) {
    collect_promotable_allocas(func);

    if (allocas_.empty()) return false;

    for (const auto& alloca : allocas_) {
      insert_phi_nodes(alloca);
    }

    std::unordered_map<AllocaPtr, ValuePtr> current_values;
    for (const auto& alloca : allocas_) {
      auto ptr_type =
          std::dynamic_pointer_cast<llvm::PointerType>(alloca->get_type());
      auto content_type = ptr_type->get_reference_type();
      current_values[alloca] = llvm::get_zero(content_type);
    }

    std::unordered_set<BlockPtr> visited;
    rename(func->entry_block(), current_values, visited);

    cleanup(func);
    return true;
  }

 private:
  void collect_promotable_allocas(const std::shared_ptr<llvm::Function>& func) {
    for (const auto& block : func->get_basic_blocks()) {
      for (const auto& inst : block->get_instructions()) {
        if (inst->get_instruction_type() ==
            llvm::Instruction::InstructionType::ALLOCA) {
          auto alloca = std::dynamic_pointer_cast<llvm::Alloca>(inst);
          if (is_promotable(alloca)) {
            allocas_.push_back(alloca);

            for (const auto& weak_user : alloca->get_users()) {
              if (auto user = weak_user.lock()) {
                auto store = std::dynamic_pointer_cast<llvm::Store>(user);
                if (store) {
                  auto parent = store->get_parent_block().lock();
                  if (parent && !util::contains(def_blocks_[alloca], parent)) {
                    def_blocks_[alloca].push_back(parent);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  void insert_phi_nodes(const AllocaPtr& alloca) {
    std::unordered_set<BlockPtr> inserted_blocks;
    std::deque<BlockPtr> worklist(def_blocks_[alloca].begin(),
                                  def_blocks_[alloca].end());

    auto ptr_type =
        std::dynamic_pointer_cast<llvm::PointerType>(alloca->get_type());
    auto content_type = ptr_type->get_reference_type();

    while (!worklist.empty()) {
      auto block = worklist.front();
      worklist.pop_front();

      for (const auto& weak_df : block->opt_info.dominance_frontier) {
        auto df = weak_df.lock();
        if (!df) continue;

        if (inserted_blocks.find(df) != inserted_blocks.end()) {
          continue;
        }

        auto phi = llvm::Phi::create(df, content_type, gen_phi_name());

        // Incoming values are filled during renaming; zero is a placeholder.
        auto zero = llvm::get_zero(content_type);
        for (const auto& weak_pred : df->opt_info.predecessors) {
          if (auto pred = weak_pred.lock()) {
            phi->add_incoming(zero, pred);
          }
        }

        df->prepend_instruction(phi);
        phi_to_alloca_[phi] = alloca;

        inserted_blocks.insert(df);

        if (!util::contains(def_blocks_[alloca], df)) {
          worklist.push_back(df);
        }
      }
    }
  }

  void rename(BlockPtr block,
              std::unordered_map<AllocaPtr, ValuePtr> current_values,
              std::unordered_set<BlockPtr>& visited) {
    if (!block || visited.count(block)) return;
    visited.insert(block);

    auto& instructions = block->get_instructions_ref();

    for (auto it = instructions.begin(); it != instructions.end(); ++it) {
      auto inst = *it;
      auto ins_type = inst->get_instruction_type();

      if (ins_type == llvm::Instruction::InstructionType::PHI) {
        auto phi = std::dynamic_pointer_cast<llvm::Phi>(inst);
        if (phi_to_alloca_.count(phi)) {
          current_values[phi_to_alloca_[phi]] = phi;
        }
      } else if (ins_type == llvm::Instruction::InstructionType::LOAD) {
        auto load = std::dynamic_pointer_cast<llvm::Load>(inst);
        auto addr = load->get_operand(0);
        auto alloca = std::dynamic_pointer_cast<llvm::Alloca>(addr);

        if (alloca && current_values.count(alloca)) {
          util::replace_all_uses_with(load, current_values[alloca]);
          instructions_to_remove_.insert(load);
        }
      } else if (ins_type == llvm::Instruction::InstructionType::STORE) {
        auto store = std::dynamic_pointer_cast<llvm::Store>(inst);
        auto addr = store->addr();
        auto alloca = std::dynamic_pointer_cast<llvm::Alloca>(addr);

        if (alloca && current_values.count(alloca)) {
          current_values[alloca] = store->val();
          instructions_to_remove_.insert(store);
        }
      }
    }

    for (const auto& weak_succ : block->opt_info.successors) {
      auto succ = weak_succ.lock();
      if (!succ) continue;

      for (auto& inst : succ->get_instructions_ref()) {
        if (inst->get_instruction_type() !=
            llvm::Instruction::InstructionType::PHI) {
          break;
        }

        auto phi = std::dynamic_pointer_cast<llvm::Phi>(inst);
        if (!phi_to_alloca_.count(phi)) continue;

        auto alloca = phi_to_alloca_[phi];
        if (current_values.count(alloca)) {
          update_phi_incoming(phi, block, current_values[alloca]);
        }
      }
    }

    for (const auto& weak_child : block->opt_info.immediate_dominated) {
      if (auto child = weak_child.lock()) {
        if (!visited.count(child)) {
          rename(child, current_values, visited);
        }
      }
    }
  }

  void cleanup(const std::shared_ptr<llvm::Function>& func) {
    for (const auto& alloca : allocas_) {
      instructions_to_remove_.insert(alloca);
    }

    for (const auto& block : func->get_basic_blocks()) {
      auto& instructions = block->get_instructions_ref();
      for (auto it = instructions.begin(); it != instructions.end();) {
        if (instructions_to_remove_.count(*it)) {
          util::remove_all_operands(*it);
          it = instructions.erase(it);
        } else {
          ++it;
        }
      }
    }
  }

 private:
  std::vector<AllocaPtr> allocas_;
  std::unordered_map<AllocaPtr, std::vector<BlockPtr>> def_blocks_;
  std::unordered_map<PhiPtr, AllocaPtr> phi_to_alloca_;
  std::unordered_set<std::shared_ptr<llvm::Instruction>>
      instructions_to_remove_;
};

bool Mem2Reg::run(llvm::Module& module) {
  bool modified = false;

  for (const auto& func : module.get_functions()) {
    Mem2RegImpl impl;
    modified |= impl.run(func);
  }

  return modified;
}

}  // namespace midend::opt::pass
