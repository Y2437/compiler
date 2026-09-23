#include <list>

#include "backend/mips/regalloc/regalloc.hpp"

namespace backend::mips {

void SimpleRegisterAllocator::allocate(MipsModule& module) {
  for (auto& func : module.get_functions()) {
    allocate_function(func);
  }
}

void SimpleRegisterAllocator::allocate_function(MipsFunctionPtr& func) {
  current_func_ = func;
  vreg_to_stack_.clear();

  // 第一遍：为所有虚拟寄存器分配栈槽
  for (auto& bb : func->get_basic_blocks()) {
    for (const auto& inst : bb->get_instructions()) {
      // 检查定义的寄存器
      for (const auto& def : inst->get_defs()) {
        if (def->is_virtual()) {
          get_stack_slot(def->get_virtual_id());
        }
      }
      // 检查使用的寄存器
      for (const auto& use : inst->get_uses()) {
        if (use->is_virtual()) {
          get_stack_slot(use->get_virtual_id());
        }
      }
    }
  }

  // 保存旧的栈帧大小
  int old_total_size = func->get_stack_frame().total_size;

  // 更新栈帧大小（现在包含了所有 spill slot）
  func->finalize_stack_frame();

  int new_total_size = func->get_stack_frame().total_size;

  // 如果栈帧大小变化了，需要修正 prologue 和 epilogue 中的立即数
  if (new_total_size != old_total_size) {
    fix_stack_frame_size(func, old_total_size, new_total_size);
  }

  // 第二遍：重写指令，将虚拟寄存器替换为物理寄存器
  for (auto& bb : func->get_basic_blocks()) {
    rewrite_instruction(bb);
  }
}

int SimpleRegisterAllocator::get_stack_slot(int vreg_id) {
  auto it = vreg_to_stack_.find(vreg_id);
  if (it != vreg_to_stack_.end()) {
    return it->second;
  }

  // 分配新的栈槽
  int offset = current_func_->alloc_spill_slot();
  vreg_to_stack_[vreg_id] = offset;
  return offset;
}

void SimpleRegisterAllocator::rewrite_instruction(MipsBasicBlockPtr& bb) {
  auto& insts = bb->get_instructions();

  for (auto it = insts.begin(); it != insts.end(); ++it) {
    auto inst = *it;

    // 获取使用和定义的虚拟寄存器
    auto uses = inst->get_uses();
    auto defs = inst->get_defs();

    // 临时寄存器分配
    // $t0 用于第一个使用的虚拟寄存器
    // $t1 用于第二个使用的虚拟寄存器
    // $t2 用于定义的虚拟寄存器

    int use_idx = 0;
    std::vector<std::pair<int, Reg>> use_mappings;  // vreg_id -> preg

    // 在指令前加载使用的虚拟寄存器
    for (auto& use : uses) {
      if (use->is_virtual()) {
        Reg temp = (use_idx == 0) ? Reg::T0 : Reg::T1;
        load_vreg(insts, it, use, temp);
        use_mappings.emplace_back(use->get_virtual_id(), temp);
        use_idx++;
      }
    }

    // 对于定义的虚拟寄存器
    std::vector<std::pair<int, Reg>> def_mappings;  // vreg_id -> preg
    for (auto& def : defs) {
      if (def->is_virtual()) {
        def_mappings.emplace_back(def->get_virtual_id(), Reg::T2);
      }
    }

    // 使用统一的 replace_reg 方法替换虚拟寄存器为物理寄存器
    for (const auto& [vreg_id, preg] : use_mappings) {
      inst->replace_reg(vreg_id, preg);
    }
    for (const auto& [vreg_id, preg] : def_mappings) {
      inst->replace_reg(vreg_id, preg);
    }

    // 在指令后保存定义的虚拟寄存器
    auto next_it = std::next(it);
    for (auto& def : defs) {
      if (def->is_virtual()) {
        store_vreg(insts, next_it, def, Reg::T2);
      }
    }
  }
}

void SimpleRegisterAllocator::load_vreg(std::list<MipsInstPtr>& insts,
                                        std::list<MipsInstPtr>::iterator& pos,
                                        std::shared_ptr<RegOperand> vreg,
                                        Reg target) {
  int offset = vreg_to_stack_[vreg->get_virtual_id()];
  auto load_inst = MemInst::create(MipsInstType::LW, RegOperand::create(target),
                                   RegOperand::create(Reg::SP), offset);
  insts.insert(pos, load_inst);
}

void SimpleRegisterAllocator::store_vreg(std::list<MipsInstPtr>& insts,
                                         std::list<MipsInstPtr>::iterator& pos,
                                         std::shared_ptr<RegOperand> vreg,
                                         Reg source) {
  int offset = vreg_to_stack_[vreg->get_virtual_id()];
  auto store_inst =
      MemInst::create(MipsInstType::SW, RegOperand::create(source),
                      RegOperand::create(Reg::SP), offset);
  insts.insert(pos, store_inst);
}

void SimpleRegisterAllocator::fix_stack_frame_size(MipsFunctionPtr& func,
                                                   int old_size, int new_size) {
  int size_diff = new_size - old_size;

  // 遍历所有基本块，找到并修正 prologue 和 epilogue 中的栈帧操作
  for (auto& bb : func->get_basic_blocks()) {
    for (auto& inst : bb->get_instructions()) {
      // 查找 addiu $sp, $sp, -old_size 或 addiu $sp, $sp, old_size
      if (auto i_inst = std::dynamic_pointer_cast<ITypeInst>(inst)) {
        if (i_inst->get_type() == MipsInstType::ADDIU) {
          auto rt = i_inst->get_rt();
          auto rs = i_inst->get_rs();
          auto imm = i_inst->get_imm();

          // 检查是否是 addiu $sp, $sp, imm
          if (rt && rs && !rt->is_virtual() && !rs->is_virtual() &&
              rt->get_reg() == Reg::SP && rs->get_reg() == Reg::SP) {
            int imm_val = imm->get_value();
            // prologue: addiu $sp, $sp, -old_size
            if (imm_val == -old_size) {
              i_inst->set_imm(ImmOperand::create(-new_size));
            }
            // epilogue: addiu $sp, $sp, old_size
            else if (imm_val == old_size) {
              i_inst->set_imm(ImmOperand::create(new_size));
            }
          }
        }
      }

      // 修复读取栈上参数的 lw 指令
      // 这些指令的偏移量 >= old_size，因为参数在调用者的栈帧中
      if (auto mem_inst = std::dynamic_pointer_cast<MemInst>(inst)) {
        if (mem_inst->get_type() == MipsInstType::LW) {
          auto mem = mem_inst->get_mem();
          auto base = mem->get_base();
          // 检查是否是 lw reg, offset($sp) 且 offset >= old_size
          if (base && !base->is_virtual() && base->get_reg() == Reg::SP) {
            int offset = mem->get_offset();
            // 如果偏移量 >= old_size，说明是访问栈上参数
            if (offset >= old_size) {
              mem->set_offset(offset + size_diff);
            }
          }
        }
      }
    }
  }
}

}  // namespace backend::mips
