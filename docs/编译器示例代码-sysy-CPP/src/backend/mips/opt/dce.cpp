#include "backend/mips/opt/dce.hpp"

#include <algorithm>

namespace backend::mips {

void DeadCodeEliminator::run() {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto& func : module_.get_functions()) {
      build_cfg(func);
      compute_liveness(func);
      changed |= eliminate_dead_instructions(func);
    }
  }
}

auto DeadCodeEliminator::get_uses(const MipsInstPtr& inst)
    -> std::unordered_set<int> {
  std::unordered_set<int> uses;
  auto regs = inst->get_uses();
  for (auto& reg : regs) {
    if (reg && reg->is_virtual()) {
      uses.insert(reg->get_virtual_id());
    }
  }
  return uses;
}

auto DeadCodeEliminator::get_defs(const MipsInstPtr& inst)
    -> std::unordered_set<int> {
  std::unordered_set<int> defs;
  auto regs = inst->get_defs();
  for (auto& reg : regs) {
    if (reg && reg->is_virtual()) {
      defs.insert(reg->get_virtual_id());
    }
  }
  return defs;
}

void DeadCodeEliminator::build_cfg(MipsFunctionPtr& func) {
  successors_.clear();

  auto& bbs = func->get_basic_blocks();

  // 构建标签到块的映射
  std::unordered_map<std::string, MipsBasicBlockPtr> label_map;
  for (auto& bb : bbs) {
    label_map[bb->get_label()] = bb;
  }

  for (auto it = bbs.begin(); it != bbs.end(); ++it) {
    auto& bb = *it;
    auto& insts = bb->get_instructions();
    std::vector<MipsBasicBlockPtr> succs;

    bool has_unconditional_jump = false;
    bool has_return = false;

    for (auto& inst : insts) {
      auto type = inst->get_type();

      switch (type) {
        case MipsInstType::J: {
          auto jump_inst = std::dynamic_pointer_cast<JumpInst>(inst);
          if (jump_inst) {
            auto target = label_map.find(jump_inst->get_label()->get_label());
            if (target != label_map.end()) {
              succs.push_back(target->second);
            }
          }
          has_unconditional_jump = true;
          break;
        }
        case MipsInstType::BEQ:
        case MipsInstType::BNE: {
          auto branch_inst = std::dynamic_pointer_cast<BranchInst>(inst);
          if (branch_inst) {
            auto target = label_map.find(branch_inst->get_label()->get_label());
            if (target != label_map.end()) {
              succs.push_back(target->second);
            }
          }
          break;
        }
        case MipsInstType::BGEZ:
        case MipsInstType::BGTZ:
        case MipsInstType::BLEZ:
        case MipsInstType::BLTZ: {
          auto branch_inst = std::dynamic_pointer_cast<BranchZeroInst>(inst);
          if (branch_inst) {
            auto target = label_map.find(branch_inst->get_label()->get_label());
            if (target != label_map.end()) {
              succs.push_back(target->second);
            }
          }
          break;
        }
        case MipsInstType::JR:
          has_return = true;
          break;
        default:
          break;
      }
    }

    // 顺序落入
    if (!has_unconditional_jump && !has_return) {
      auto next_it = std::next(it);
      if (next_it != bbs.end()) {
        succs.push_back(*next_it);
      }
    }

    successors_[bb] = succs;
  }
}

void DeadCodeEliminator::compute_liveness(MipsFunctionPtr& func) {
  liveness_map_.clear();
  auto& bbs = func->get_basic_blocks();

  for (auto& bb : bbs) {
    LivenessInfo info;

    for (auto& inst : bb->get_instructions()) {
      auto uses = get_uses(inst);
      auto defs = get_defs(inst);

      for (int use : uses) {
        if (info.live_def.find(use) == info.live_def.end()) {
          info.live_use.insert(use);
        }
      }

      for (int def : defs) {
        info.live_def.insert(def);
      }
    }

    liveness_map_[bb] = info;
  }

  bool changed = true;
  while (changed) {
    changed = false;

    for (auto rit = bbs.rbegin(); rit != bbs.rend(); ++rit) {
      auto& bb = *rit;
      auto& info = liveness_map_[bb];

      auto old_in = info.live_in;
      auto old_out = info.live_out;

      // live_out = union(successor.live_in)
      std::unordered_set<int> new_out;
      auto& succs = successors_[bb];
      for (auto& succ : succs) {
        auto& succ_info = liveness_map_[succ];
        new_out.insert(succ_info.live_in.begin(), succ_info.live_in.end());
      }

      // live_in = use union (live_out - def)
      std::unordered_set<int> new_in = info.live_use;
      for (int v : new_out) {
        if (info.live_def.find(v) == info.live_def.end()) {
          new_in.insert(v);
        }
      }

      if (new_in != info.live_in || new_out != info.live_out) {
        info.live_in = std::move(new_in);
        info.live_out = std::move(new_out);
        changed = true;
      }
    }
  }
}

bool DeadCodeEliminator::has_side_effects(const MipsInstPtr& inst) {
  auto type = inst->get_type();

  switch (type) {
    case MipsInstType::SW:
    case MipsInstType::SB:
    case MipsInstType::J:
    case MipsInstType::JAL:
    case MipsInstType::JR:
    case MipsInstType::JALR:
    case MipsInstType::BEQ:
    case MipsInstType::BNE:
    case MipsInstType::BGEZ:
    case MipsInstType::BGTZ:
    case MipsInstType::BLEZ:
    case MipsInstType::BLTZ:
    case MipsInstType::SYSCALL:
    case MipsInstType::LABEL:
    case MipsInstType::COMMENT:
      return true;
    default:
      return false;
  }
}

bool DeadCodeEliminator::can_delete_instruction(const MipsInstPtr& inst) {
  if (has_side_effects(inst)) {
    return false;
  }

  auto defs = get_defs(inst);
  if (defs.empty()) {
    return false;
  }

  return true;
}

bool DeadCodeEliminator::eliminate_dead_instructions(MipsFunctionPtr& func) {
  bool changed = false;

  for (auto& bb : func->get_basic_blocks()) {
    auto& insts = bb->get_instructions();

    std::unordered_set<int> live = liveness_map_[bb].live_out;

    for (auto it = insts.rbegin(); it != insts.rend();) {
      auto inst = *it;

      if (can_delete_instruction(inst)) {
        auto defs = get_defs(inst);

        bool all_dead = true;
        for (int def : defs) {
          if (live.find(def) != live.end()) {
            all_dead = false;
            break;
          }
        }

        if (all_dead) {
          auto forward_it = std::next(it).base();
          insts.erase(forward_it);
          changed = true;
          continue;
        }
      }

      auto defs = get_defs(inst);
      auto uses = get_uses(inst);

      for (int def : defs) {
        live.erase(def);
      }

      for (int use : uses) {
        live.insert(use);
      }

      ++it;
    }
  }

  return changed;
}

}  // namespace backend::mips
