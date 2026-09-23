#include "backend/mips/opt/cfg_opt.hpp"

#include <algorithm>
#include <queue>
#include <unordered_map>

namespace backend::mips {

void CFGOptimizer::run() {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto& func : module_.get_functions()) {
      changed |= redirect_goto(func);
      changed |= remove_unreachable_blocks(func);
      changed |= remove_empty_blocks(func);
    }
  }
}

auto CFGOptimizer::get_jump_target(const MipsBasicBlockPtr& bb) -> std::string {
  auto& insts = bb->get_instructions();
  if (insts.empty()) return "";

  auto last_inst = insts.back();
  if (last_inst->get_type() == MipsInstType::J) {
    auto jump_inst = std::dynamic_pointer_cast<JumpInst>(last_inst);
    if (jump_inst) {
      return jump_inst->get_label()->get_label();
    }
  }

  return "";
}

auto CFGOptimizer::find_block_by_label(MipsFunctionPtr& func,
                                       const std::string& label)
    -> MipsBasicBlockPtr {
  for (auto& bb : func->get_basic_blocks()) {
    if (bb->get_label() == label) {
      return bb;
    }
  }
  return nullptr;
}

bool CFGOptimizer::is_jump_only_block(const MipsBasicBlockPtr& bb) {
  auto& insts = bb->get_instructions();
  if (insts.size() != 1) return false;

  auto inst = insts.front();
  return inst->get_type() == MipsInstType::J;
}

void CFGOptimizer::update_jump_targets(MipsFunctionPtr& func,
                                       const std::string& old_label,
                                       const std::string& new_label) {
  for (auto& bb : func->get_basic_blocks()) {
    for (auto it = bb->get_instructions().begin();
         it != bb->get_instructions().end(); ++it) {
      auto inst = *it;

      // 更新无条件跳转
      if (inst->get_type() == MipsInstType::J) {
        auto jump_inst = std::dynamic_pointer_cast<JumpInst>(inst);
        if (jump_inst && jump_inst->get_label()->get_label() == old_label) {
          *it = JumpInst::create(MipsInstType::J, new_label);
        }
      }
      // 更新条件分支
      else if (inst->get_type() == MipsInstType::BEQ ||
               inst->get_type() == MipsInstType::BNE) {
        auto branch_inst = std::dynamic_pointer_cast<BranchInst>(inst);
        if (branch_inst && branch_inst->get_label()->get_label() == old_label) {
          *it = BranchInst::create(inst->get_type(), branch_inst->get_rs(),
                                   branch_inst->get_rt(), new_label);
        }
      } else if (inst->get_type() == MipsInstType::BGEZ ||
                 inst->get_type() == MipsInstType::BGTZ ||
                 inst->get_type() == MipsInstType::BLEZ ||
                 inst->get_type() == MipsInstType::BLTZ) {
        auto branch_inst = std::dynamic_pointer_cast<BranchZeroInst>(inst);
        if (branch_inst && branch_inst->get_label()->get_label() == old_label) {
          *it = BranchZeroInst::create(inst->get_type(), branch_inst->get_rs(),
                                       new_label);
        }
      }
    }
  }
}

auto CFGOptimizer::get_reachable_blocks(MipsFunctionPtr& func)
    -> std::unordered_set<MipsBasicBlockPtr> {
  std::unordered_set<MipsBasicBlockPtr> reachable;
  std::queue<MipsBasicBlockPtr> worklist;

  auto& bbs = func->get_basic_blocks();
  if (bbs.empty()) return reachable;

  // 从入口块开始
  auto entry = bbs.front();
  worklist.push(entry);
  reachable.insert(entry);

  while (!worklist.empty()) {
    auto bb = worklist.front();
    worklist.pop();

    auto& insts = bb->get_instructions();

    // 查找所有可能的后继
    for (auto& inst : insts) {
      std::string target_label;

      if (inst->get_type() == MipsInstType::J) {
        auto jump_inst = std::dynamic_pointer_cast<JumpInst>(inst);
        if (jump_inst) target_label = jump_inst->get_label()->get_label();
      } else if (inst->get_type() == MipsInstType::BEQ ||
                 inst->get_type() == MipsInstType::BNE) {
        auto branch_inst = std::dynamic_pointer_cast<BranchInst>(inst);
        if (branch_inst) target_label = branch_inst->get_label()->get_label();
      } else if (inst->get_type() == MipsInstType::BGEZ ||
                 inst->get_type() == MipsInstType::BGTZ ||
                 inst->get_type() == MipsInstType::BLEZ ||
                 inst->get_type() == MipsInstType::BLTZ) {
        auto branch_inst = std::dynamic_pointer_cast<BranchZeroInst>(inst);
        if (branch_inst) target_label = branch_inst->get_label()->get_label();
      }

      if (!target_label.empty()) {
        auto target = find_block_by_label(func, target_label);
        if (target && reachable.find(target) == reachable.end()) {
          reachable.insert(target);
          worklist.push(target);
        }
      }
    }

    // 检查顺序落入
    bool has_unconditional_jump = false;
    bool has_return = false;
    if (!insts.empty()) {
      auto last_type = insts.back()->get_type();
      has_unconditional_jump = (last_type == MipsInstType::J);
      has_return = (last_type == MipsInstType::JR);
    }

    if (!has_unconditional_jump && !has_return) {
      // 找到当前块在列表中的位置
      for (auto it = bbs.begin(); it != bbs.end(); ++it) {
        if (*it == bb) {
          auto next_it = std::next(it);
          if (next_it != bbs.end()) {
            auto next_bb = *next_it;
            if (reachable.find(next_bb) == reachable.end()) {
              reachable.insert(next_bb);
              worklist.push(next_bb);
            }
          }
          break;
        }
      }
    }
  }

  return reachable;
}

bool CFGOptimizer::redirect_goto(MipsFunctionPtr& func) {
  bool changed = false;

  // 构建重定向映射
  std::unordered_map<std::string, std::string> redirect_map;

  for (auto& bb : func->get_basic_blocks()) {
    if (is_jump_only_block(bb)) {
      std::string target = get_jump_target(bb);
      if (!target.empty() && target != bb->get_label()) {
        redirect_map[bb->get_label()] = target;
      }
    }
  }

  if (redirect_map.empty()) return false;

  // 解决传递性重定向
  bool updated = true;
  while (updated) {
    updated = false;
    for (auto& [from, to] : redirect_map) {
      auto it = redirect_map.find(to);
      if (it != redirect_map.end() && it->second != to) {
        redirect_map[from] = it->second;
        updated = true;
      }
    }
  }

  // 应用重定向
  for (auto& bb : func->get_basic_blocks()) {
    for (auto it = bb->get_instructions().begin();
         it != bb->get_instructions().end(); ++it) {
      auto inst = *it;

      if (inst->get_type() == MipsInstType::J) {
        auto jump_inst = std::dynamic_pointer_cast<JumpInst>(inst);
        if (jump_inst) {
          std::string label = jump_inst->get_label()->get_label();
          auto map_it = redirect_map.find(label);
          if (map_it != redirect_map.end()) {
            *it = JumpInst::create(MipsInstType::J, map_it->second);
            changed = true;
          }
        }
      } else if (inst->get_type() == MipsInstType::BEQ ||
                 inst->get_type() == MipsInstType::BNE) {
        auto branch_inst = std::dynamic_pointer_cast<BranchInst>(inst);
        if (branch_inst) {
          std::string label = branch_inst->get_label()->get_label();
          auto map_it = redirect_map.find(label);
          if (map_it != redirect_map.end()) {
            *it = BranchInst::create(inst->get_type(), branch_inst->get_rs(),
                                     branch_inst->get_rt(), map_it->second);
            changed = true;
          }
        }
      } else if (inst->get_type() == MipsInstType::BGEZ ||
                 inst->get_type() == MipsInstType::BGTZ ||
                 inst->get_type() == MipsInstType::BLEZ ||
                 inst->get_type() == MipsInstType::BLTZ) {
        auto branch_inst = std::dynamic_pointer_cast<BranchZeroInst>(inst);
        if (branch_inst) {
          std::string label = branch_inst->get_label()->get_label();
          auto map_it = redirect_map.find(label);
          if (map_it != redirect_map.end()) {
            *it = BranchZeroInst::create(inst->get_type(),
                                         branch_inst->get_rs(), map_it->second);
            changed = true;
          }
        }
      }
    }
  }

  return changed;
}

bool CFGOptimizer::remove_unreachable_blocks(MipsFunctionPtr& func) {
  auto reachable = get_reachable_blocks(func);
  auto& bbs = func->get_basic_blocks();

  bool changed = false;
  for (auto it = bbs.begin(); it != bbs.end();) {
    if (reachable.find(*it) == reachable.end()) {
      it = bbs.erase(it);
      changed = true;
    } else {
      ++it;
    }
  }

  return changed;
}

bool CFGOptimizer::remove_empty_blocks(MipsFunctionPtr& func) {
  bool changed = false;
  auto& bbs = func->get_basic_blocks();

  // 不删除第一个块（入口块）
  for (auto it = std::next(bbs.begin()); it != bbs.end();) {
    auto& bb = *it;
    if (bb->get_instructions().empty()) {
      // 找到下一个非空块
      auto next_it = std::next(it);
      if (next_it != bbs.end()) {
        // 更新所有跳转到当前块的指令
        update_jump_targets(func, bb->get_label(), (*next_it)->get_label());
      }
      it = bbs.erase(it);
      changed = true;
    } else {
      ++it;
    }
  }

  return changed;
}

}  // namespace backend::mips
