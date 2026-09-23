#include "backend/mips/opt/peephole.hpp"

#include <algorithm>
#include <iterator>
#include <unordered_map>

namespace backend::mips {

void PeepholeOptimizer::run_before_ra() {
  bool changed = true;
  int iteration = 0;
  constexpr int kMaxIterations = 100;

  while (changed && iteration < kMaxIterations) {
    changed = false;
    iteration++;

    for (auto& func : module_.get_functions()) {
      for (auto& bb : func->get_basic_blocks()) {
        changed |= remove_same_reg_move(bb);
        changed |= optimize_addiu_zero(bb);
        changed |= optimize_li_zero(bb);
        changed |= optimize_add_zero(bb);

        changed |= remove_useless_load_store(bb);
        changed |= optimize_store_load(bb);

        changed |= optimize_mul_power_of_two(bb);
      }
    }
  }

  // Run once to avoid repeatedly replacing constants with moves.
  for (auto& func : module_.get_functions()) {
    for (auto& bb : func->get_basic_blocks()) {
      reuse_constant_value(bb);
    }
  }
}

bool PeepholeOptimizer::remove_same_reg_move(MipsBasicBlockPtr& bb) {
  bool changed = false;
  auto& insts = bb->get_instructions();

  for (auto it = insts.begin(); it != insts.end();) {
    auto inst = *it;

    if (inst->get_type() == MipsInstType::MOVE) {
      auto move_inst = std::dynamic_pointer_cast<MoveInst>(inst);
      if (move_inst) {
        auto rd = move_inst->get_rd();
        auto rs = move_inst->get_rs();

        // 检查源和目标是否是同一个寄存器
        if (rd && rs) {
          bool same_reg = false;

          // 如果都是物理寄存器
          if (!rd->is_virtual() && !rs->is_virtual()) {
            same_reg = (rd->get_reg() == rs->get_reg());
          }
          // 如果都是虚拟寄存器
          else if (rd->is_virtual() && rs->is_virtual()) {
            same_reg = (rd->get_virtual_id() == rs->get_virtual_id());
          }

          if (same_reg) {
            it = insts.erase(it);
            changed = true;
            continue;
          }
        }
      }
    }

    ++it;
  }

  return changed;
}

bool PeepholeOptimizer::optimize_addiu_zero(MipsBasicBlockPtr& bb) {
  bool changed = false;
  auto& insts = bb->get_instructions();

  for (auto it = insts.begin(); it != insts.end();) {
    auto inst = *it;

    if (inst->get_type() == MipsInstType::ADDIU) {
      auto itype_inst = std::dynamic_pointer_cast<ITypeInst>(inst);
      if (itype_inst) {
        auto imm = itype_inst->get_imm();
        if (imm && imm->get_value() == 0) {
          auto rt = itype_inst->get_rt();
          auto rs = itype_inst->get_rs();

          // 如果源和目标相同，直接删除
          bool same_reg = false;
          if (rt && rs) {
            if (!rt->is_virtual() && !rs->is_virtual()) {
              same_reg = (rt->get_reg() == rs->get_reg());
            } else if (rt->is_virtual() && rs->is_virtual()) {
              same_reg = (rt->get_virtual_id() == rs->get_virtual_id());
            }
          }

          if (same_reg) {
            it = insts.erase(it);
            changed = true;
            continue;
          } else {
            // 替换为 move 指令
            *it = MoveInst::create(rt, rs);
            changed = true;
          }
        }
      }
    }

    ++it;
  }

  return changed;
}

bool PeepholeOptimizer::optimize_li_zero(MipsBasicBlockPtr& bb) {
  bool changed = false;
  auto& insts = bb->get_instructions();

  for (auto it = insts.begin(); it != insts.end(); ++it) {
    auto inst = *it;

    if (inst->get_type() == MipsInstType::LI) {
      auto li_inst = std::dynamic_pointer_cast<LiInst>(inst);
      if (li_inst) {
        auto imm = li_inst->get_imm();
        if (imm && imm->get_value() == 0) {
          auto rd = li_inst->get_rd();
          // 替换为 move $rd, $zero
          *it = MoveInst::create(rd, RegOperand::create(Reg::ZERO));
          changed = true;
        }
      }
    }
  }

  return changed;
}

bool PeepholeOptimizer::optimize_add_zero(MipsBasicBlockPtr& bb) {
  bool changed = false;
  auto& insts = bb->get_instructions();

  for (auto it = insts.begin(); it != insts.end(); ++it) {
    auto inst = *it;

    if (inst->get_type() == MipsInstType::ADDU) {
      auto rtype_inst = std::dynamic_pointer_cast<RTypeInst>(inst);
      if (rtype_inst) {
        auto rd = rtype_inst->get_rd();
        auto rs = rtype_inst->get_rs();
        auto rt = rtype_inst->get_rt();

        // 检查 rs 是否是 $zero
        if (rs && !rs->is_virtual() && rs->get_reg() == Reg::ZERO) {
          *it = MoveInst::create(rd, rt);
          changed = true;
          continue;
        }

        // 检查 rt 是否是 $zero
        if (rt && !rt->is_virtual() && rt->get_reg() == Reg::ZERO) {
          *it = MoveInst::create(rd, rs);
          changed = true;
          continue;
        }
      }
    }
  }

  return changed;
}

bool PeepholeOptimizer::same_memory_location(
    const std::shared_ptr<MemOperand>& a,
    const std::shared_ptr<MemOperand>& b) {
  if (!a || !b) return false;

  // 检查偏移量是否相同
  if (a->get_offset() != b->get_offset()) return false;

  // 检查基址寄存器是否相同
  auto base_a = a->get_base();
  auto base_b = b->get_base();

  if (!base_a || !base_b) return false;

  if (!base_a->is_virtual() && !base_b->is_virtual()) {
    return base_a->get_reg() == base_b->get_reg();
  } else if (base_a->is_virtual() && base_b->is_virtual()) {
    return base_a->get_virtual_id() == base_b->get_virtual_id();
  }

  return false;
}

bool PeepholeOptimizer::remove_useless_load_store(MipsBasicBlockPtr& bb) {
  bool changed = false;
  auto& insts = bb->get_instructions();

  if (insts.size() < 2) return false;

  for (auto it = std::next(insts.begin()); it != insts.end();) {
    auto current = *it;
    auto prev_it = std::prev(it);
    auto prev = *prev_it;

    // 检查连续的相同内存访问
    bool both_memory = false;
    bool both_load = false;
    bool both_store = false;

    auto curr_type = current->get_type();
    auto prev_type = prev->get_type();

    // 判断是否都是load或都是store
    if ((curr_type == MipsInstType::LW || curr_type == MipsInstType::LB) &&
        (prev_type == MipsInstType::LW || prev_type == MipsInstType::LB)) {
      both_memory = true;
      both_load = true;
    } else if ((curr_type == MipsInstType::SW ||
                curr_type == MipsInstType::SB) &&
               (prev_type == MipsInstType::SW ||
                prev_type == MipsInstType::SB)) {
      both_memory = true;
      both_store = true;
    }

    if (both_memory) {
      auto curr_mem = std::dynamic_pointer_cast<MemInst>(current);
      auto prev_mem = std::dynamic_pointer_cast<MemInst>(prev);

      if (curr_mem && prev_mem) {
        auto curr_rt = curr_mem->get_rt();
        auto prev_rt = prev_mem->get_rt();
        auto curr_m = curr_mem->get_mem();
        auto prev_m = prev_mem->get_mem();

        // 检查是否访问相同地址和相同寄存器
        if (same_memory_location(curr_m, prev_m)) {
          bool same_rt = false;
          if (curr_rt && prev_rt) {
            if (!curr_rt->is_virtual() && !prev_rt->is_virtual()) {
              same_rt = (curr_rt->get_reg() == prev_rt->get_reg());
            } else if (curr_rt->is_virtual() && prev_rt->is_virtual()) {
              same_rt =
                  (curr_rt->get_virtual_id() == prev_rt->get_virtual_id());
            }
          }

          if (same_rt) {
            // 删除重复的指令
            it = insts.erase(it);
            changed = true;
            continue;
          }
        }
      }
    }

    ++it;
  }

  return changed;
}

bool PeepholeOptimizer::optimize_store_load(MipsBasicBlockPtr& bb) {
  bool changed = false;
  auto& insts = bb->get_instructions();

  if (insts.size() < 2) return false;

  for (auto it = std::next(insts.begin()); it != insts.end();) {
    auto current = *it;
    auto prev_it = std::prev(it);
    auto prev = *prev_it;

    bool is_store_load = false;

    // 检查 store 后接 load
    if ((prev->get_type() == MipsInstType::SW &&
         current->get_type() == MipsInstType::LW) ||
        (prev->get_type() == MipsInstType::SB &&
         current->get_type() == MipsInstType::LB)) {
      is_store_load = true;
    }

    if (is_store_load) {
      auto store_inst = std::dynamic_pointer_cast<MemInst>(prev);
      auto load_inst = std::dynamic_pointer_cast<MemInst>(current);

      if (store_inst && load_inst) {
        auto store_mem = store_inst->get_mem();
        auto load_mem = load_inst->get_mem();

        // 检查是否访问相同内存地址
        if (same_memory_location(store_mem, load_mem)) {
          auto store_rt = store_inst->get_rt();  // 被存储的值
          auto load_rt = load_inst->get_rt();    // load的目标

          // 用 move 替换 load: move load_rt, store_rt
          *it = MoveInst::create(load_rt, store_rt);
          changed = true;
        }
      }
    }

    ++it;
  }

  return changed;
}

bool PeepholeOptimizer::reuse_constant_value(MipsBasicBlockPtr& bb) {
  bool changed = false;
  auto& insts = bb->get_instructions();

  const int REUSE_RANGE = 16;  // 常量复用的范围

  // 值 -> (寄存器, 距离)
  std::unordered_map<int, std::pair<std::shared_ptr<RegOperand>, int>>
      const_map;

  int distance = 0;
  for (auto it = insts.begin(); it != insts.end(); ++it, ++distance) {
    auto inst = *it;

    // 函数调用后清空常量映射，因为函数可能修改静态变量等状态
    // JAL 指令会调用函数，JALR 是间接调用
    if (inst->get_type() == MipsInstType::JAL ||
        inst->get_type() == MipsInstType::JALR) {
      const_map.clear();
      continue;
    }

    // 清理过期的常量
    for (auto map_it = const_map.begin(); map_it != const_map.end();) {
      if (distance - map_it->second.second > REUSE_RANGE) {
        map_it = const_map.erase(map_it);
      } else {
        ++map_it;
      }
    }

    // 如果指令定义了某个寄存器，先清除使用该寄存器的常量
    auto defs = inst->get_defs();
    for (auto& def : defs) {
      if (def && def->is_virtual()) {
        for (auto map_it = const_map.begin(); map_it != const_map.end();) {
          auto& [reg, _] = map_it->second;
          if (reg->is_virtual() &&
              reg->get_virtual_id() == def->get_virtual_id()) {
            map_it = const_map.erase(map_it);
          } else {
            ++map_it;
          }
        }
      }
    }

    // 检查是否是 LI 指令
    if (inst->get_type() == MipsInstType::LI) {
      auto li_inst = std::dynamic_pointer_cast<LiInst>(inst);
      if (li_inst) {
        auto rd = li_inst->get_rd();
        auto imm = li_inst->get_imm();

        if (rd && imm && rd->is_virtual()) {
          int value = imm->get_value();

          // 检查是否有相同的常量可以复用
          auto map_it = const_map.find(value);
          if (map_it != const_map.end()) {
            auto& [existing_reg, _] = map_it->second;
            // 确保existing_reg仍然有效（没有被重定义）
            // 用 move 替换 li
            *it = MoveInst::create(rd, existing_reg);
            changed = true;
            // 注意：不要把rd加入const_map，因为它现在是move的目标
          } else {
            // 记录这个常量
            const_map[value] = {rd, distance};
          }
        }
      }
    }
  }

  return changed;
}

bool PeepholeOptimizer::optimize_mul_power_of_two(MipsBasicBlockPtr& bb) {
  bool changed = false;
  auto& insts = bb->get_instructions();

  if (insts.size() < 2) return false;

  for (auto it = std::next(insts.begin()); it != insts.end(); ++it) {
    auto current = *it;
    auto prev_it = std::prev(it);
    auto prev = *prev_it;

    // 检查 li 后接 mul 模式
    if (prev->get_type() == MipsInstType::LI &&
        current->get_type() == MipsInstType::MUL) {
      auto li_inst = std::dynamic_pointer_cast<LiInst>(prev);
      auto mul_inst = std::dynamic_pointer_cast<RTypeInst>(current);

      if (li_inst && mul_inst) {
        auto li_rd = li_inst->get_rd();
        auto imm = li_inst->get_imm();

        if (imm && is_power_of_two(imm->get_value())) {
          auto mul_rd = mul_inst->get_rd();
          auto mul_rs = mul_inst->get_rs();
          auto mul_rt = mul_inst->get_rt();

          // 检查 li 的寄存器是否用于乘法
          std::shared_ptr<RegOperand> other_reg = nullptr;
          bool same_as_rs = false;
          bool same_as_rt = false;

          if (li_rd && mul_rs) {
            if ((li_rd->is_virtual() && mul_rs->is_virtual() &&
                 li_rd->get_virtual_id() == mul_rs->get_virtual_id()) ||
                (!li_rd->is_virtual() && !mul_rs->is_virtual() &&
                 li_rd->get_reg() == mul_rs->get_reg())) {
              same_as_rs = true;
              other_reg = mul_rt;
            }
          }

          if (li_rd && mul_rt) {
            if ((li_rd->is_virtual() && mul_rt->is_virtual() &&
                 li_rd->get_virtual_id() == mul_rt->get_virtual_id()) ||
                (!li_rd->is_virtual() && !mul_rt->is_virtual() &&
                 li_rd->get_reg() == mul_rt->get_reg())) {
              same_as_rt = true;
              other_reg = mul_rs;
            }
          }

          if ((same_as_rs || same_as_rt) && other_reg) {
            int shift = log2_int(imm->get_value());
            // 删除 li，将 mul 替换为 sll
            insts.erase(prev_it);
            *it =
                ShiftInst::create(MipsInstType::SLL, mul_rd, other_reg, shift);
            changed = true;
          }
        }
      }
    }
  }

  return changed;
}

}  // namespace backend::mips
