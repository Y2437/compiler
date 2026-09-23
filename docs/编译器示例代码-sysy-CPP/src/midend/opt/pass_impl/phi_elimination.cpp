#include <algorithm>
#include <unordered_set>

#include "midend/llvm/instruction.hpp"
#include "midend/opt/opt_util.hpp"
#include "midend/opt/pass.hpp"

namespace midend::opt::pass {

static bool values_equal(const std::shared_ptr<llvm::Value>& v1,
                         const std::shared_ptr<llvm::Value>& v2) {
  if (v1 == v2) return true;

  auto c1 = std::dynamic_pointer_cast<llvm::ConstantInt>(v1);
  auto c2 = std::dynamic_pointer_cast<llvm::ConstantInt>(v2);
  if (c1 && c2) {
    return c1->get_val() == c2->get_val();
  }

  return false;
}

static bool try_eliminate_phi(
    const std::shared_ptr<llvm::Phi>& phi,
    std::unordered_set<std::shared_ptr<llvm::Phi>>& deleted_phis) {
  if (deleted_phis.count(phi)) return false;

  size_t num_incoming = phi->get_num_incoming();

  if (num_incoming == 1) {
    auto value = phi->get_incoming_value(0);
    deleted_phis.insert(phi);
    util::replace_all_uses_with(phi, value);
    return true;
  }

  std::shared_ptr<llvm::Value> unique_value = nullptr;

  for (size_t i = 0; i < num_incoming; ++i) {
    auto incoming_val = phi->get_incoming_value(i);

    if (incoming_val == phi) {
      continue;
    }

    if (!unique_value) {
      unique_value = incoming_val;
    } else if (!values_equal(unique_value, incoming_val)) {
      return false;
    }
  }

  if (!unique_value) {
    return false;
  }

  deleted_phis.insert(phi);
  util::replace_all_uses_with(phi, unique_value);
  return true;
}

bool PhiElimination::run(llvm::Module& module) {
  bool modified = false;
  std::unordered_set<std::shared_ptr<llvm::Phi>> deleted_phis;

  for (const auto& func : module.get_functions()) {
    std::vector<std::shared_ptr<llvm::Phi>> phis_to_check;

    for (const auto& block : func->get_basic_blocks()) {
      for (const auto& inst : block->get_instructions()) {
        if (inst->get_instruction_type() ==
            llvm::Instruction::InstructionType::PHI) {
          if (auto phi = std::dynamic_pointer_cast<llvm::Phi>(inst)) {
            phis_to_check.push_back(phi);
          }
        }
      }
    }

    for (const auto& phi : phis_to_check) {
      if (try_eliminate_phi(phi, deleted_phis)) {
        modified = true;
      }
    }

    for (const auto& block : func->get_basic_blocks()) {
      auto& instructions = block->get_instructions_ref();
      for (auto it = instructions.begin(); it != instructions.end();) {
        auto phi = std::dynamic_pointer_cast<llvm::Phi>(*it);
        if (phi && deleted_phis.count(phi)) {
          util::remove_all_operands(phi);
          it = instructions.erase(it);
        } else {
          ++it;
        }
      }
    }
  }

  deleted_phis.clear();
  return modified;
}

}  // namespace midend::opt::pass
