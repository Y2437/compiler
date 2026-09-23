#include <algorithm>
#include <list>
#include <set>
#include <unordered_set>
#include <vector>

#include "backend/mips/ir/register.hpp"
#include "backend/mips/regalloc/regalloc.hpp"

namespace backend::mips {
namespace {

std::vector<Reg> caller_saved_regs() {
  return {Reg::A0, Reg::A1, Reg::A2, Reg::A3, Reg::T0, Reg::T1,
          Reg::T2, Reg::T3, Reg::T4, Reg::T5, Reg::T6, Reg::T7,
          Reg::T8, Reg::T9, Reg::V0, Reg::V1};
}

std::vector<Reg> allocatable_regs() { return get_allocatable_regs(); }

}  // namespace

void GraphColoringRegisterAllocator::allocate(MipsModule& module) {
  // Spilled values and incoming stack arguments use overlapping offsets
  // until the final stack frame is known. Fall back as a whole when spilling
  // is required, so no partially rewritten function escapes this allocator.
  for (const auto& func : module.get_functions()) {
    auto bbs = collect_blocks(func);
    auto label_map = build_label_map(bbs);
    auto succs = build_successors(bbs, label_map);
    std::unordered_set<int> all_vregs;
    std::unordered_set<int> live_across_call;
    auto liveness = compute_liveness(bbs, succs, all_vregs, live_across_call);
    std::unordered_set<int> nodes;
    auto graph = build_interference(bbs, liveness, live_across_call,
                                    func->get_virtual_reg_count(), nodes);
    auto [colors, spilled] = color_graph(graph, nodes, live_across_call);
    if (!spilled.empty()) {
      SimpleRegisterAllocator fallback;
      fallback.allocate(module);
      return;
    }
  }

  for (auto& func : module.get_functions()) {
    allocate_function(func);
  }
}

void GraphColoringRegisterAllocator::allocate_function(MipsFunctionPtr& func) {
  bool done = false;
  while (!done) {
    auto bbs = collect_blocks(func);
    auto label_map = build_label_map(bbs);
    auto succs = build_successors(bbs, label_map);

    std::unordered_set<int> all_vregs;
    std::unordered_set<int> live_across_call;
    auto liveness = compute_liveness(bbs, succs, all_vregs, live_across_call);

    std::unordered_set<int> nodes;
    auto graph = build_interference(bbs, liveness, live_across_call,
                                    func->get_virtual_reg_count(), nodes);
    auto [colors, spilled] = color_graph(graph, nodes, live_across_call);

    if (!spilled.empty()) {
      rewrite_spills(func, spilled);
      continue;  // 重新进行一次分配
    }

    // 应用上色结果
    apply_colors(func, colors);

    // 标记需要保存的callee-saved寄存器
    for (const auto& [vreg, reg] : colors) {
      if (is_saved_reg(reg)) {
        func->add_saved_reg(reg);
      }
    }

    // 修正栈帧大小与序言/尾声
    StackFrame old_frame = func->get_stack_frame();
    func->finalize_stack_frame();
    StackFrame new_frame = func->get_stack_frame();
    fix_stack_frame(func, old_frame, new_frame);
    insert_saved_reg_saves(func);
    insert_saved_reg_restores(func);

    done = true;
  }
}

std::vector<MipsBasicBlockPtr> GraphColoringRegisterAllocator::collect_blocks(
    const MipsFunctionPtr& func) const {
  std::vector<MipsBasicBlockPtr> bbs;
  for (auto& bb : func->get_basic_blocks()) {
    bbs.push_back(bb);
  }
  return bbs;
}

std::unordered_map<std::string, MipsBasicBlockPtr>
GraphColoringRegisterAllocator::build_label_map(
    const std::vector<MipsBasicBlockPtr>& bbs) const {
  std::unordered_map<std::string, MipsBasicBlockPtr> mp;
  for (auto& bb : bbs) {
    mp[bb->get_label()] = bb;
  }
  return mp;
}

std::vector<std::vector<int>> GraphColoringRegisterAllocator::build_successors(
    const std::vector<MipsBasicBlockPtr>& bbs,
    const std::unordered_map<std::string, MipsBasicBlockPtr>& label_map) const {
  std::vector<std::vector<int>> succs(bbs.size());

  auto find_bb_index = [&](const std::string& label) -> int {
    auto it = label_map.find(label);
    if (it == label_map.end()) return -1;
    for (size_t i = 0; i < bbs.size(); ++i) {
      if (bbs[i] == it->second) return static_cast<int>(i);
    }
    return -1;
  };

  for (size_t i = 0; i < bbs.size(); ++i) {
    auto& bb = bbs[i];
    auto& insts = bb->get_instructions();
    if (insts.empty()) {
      if (i + 1 < bbs.size()) succs[i].push_back(static_cast<int>(i + 1));
      continue;
    }

    // 扫描所有指令，收集所有跳转目标
    // 这样可以处理条件分支后跟无条件跳转的情况
    bool has_unconditional_jump = false;
    bool has_return = false;

    for (const auto& inst : insts) {
      auto type = inst->get_type();
      switch (type) {
        case MipsInstType::J: {
          if (auto j = std::dynamic_pointer_cast<JumpInst>(inst)) {
            int idx = find_bb_index(j->get_label()->get_label());
            if (idx >= 0) succs[i].push_back(idx);
          }
          has_unconditional_jump = true;
          break;
        }
        case MipsInstType::JAL: {
          // 函数调用，继续执行
          break;
        }
        case MipsInstType::BEQ:
        case MipsInstType::BNE: {
          if (auto b = std::dynamic_pointer_cast<BranchInst>(inst)) {
            int idx = find_bb_index(b->get_label()->get_label());
            if (idx >= 0) succs[i].push_back(idx);
          }
          break;
        }
        case MipsInstType::BGEZ:
        case MipsInstType::BGTZ:
        case MipsInstType::BLEZ:
        case MipsInstType::BLTZ: {
          if (auto b = std::dynamic_pointer_cast<BranchZeroInst>(inst)) {
            int idx = find_bb_index(b->get_label()->get_label());
            if (idx >= 0) succs[i].push_back(idx);
          }
          break;
        }
        case MipsInstType::JR:
          has_return = true;
          break;
        case MipsInstType::JALR:
          // 间接调用，继续执行
          break;
        case MipsInstType::SYSCALL:
          // syscall 可能是 exit，保守处理
          break;
        default:
          break;
      }
    }

    // 如果没有无条件跳转也没有返回，添加 fall-through
    if (!has_unconditional_jump && !has_return) {
      if (i + 1 < bbs.size()) succs[i].push_back(static_cast<int>(i + 1));
    }
  }

  return succs;
}

std::vector<GraphColoringRegisterAllocator::BlockLiveness>
GraphColoringRegisterAllocator::compute_liveness(
    const std::vector<MipsBasicBlockPtr>& bbs,
    const std::vector<std::vector<int>>& succs,
    std::unordered_set<int>& all_vregs,
    std::unordered_set<int>& live_across_call) const {
  std::vector<BlockLiveness> info(bbs.size());

  for (size_t i = 0; i < bbs.size(); ++i) {
    auto& bb = bbs[i];
    auto& blk = info[i];
    for (const auto& inst : bb->get_instructions()) {
      for (auto& use : get_virtuals(inst->get_uses())) {
        if (!blk.def.count(use->get_virtual_id()))
          blk.use.insert(use->get_virtual_id());
        all_vregs.insert(use->get_virtual_id());
      }
      for (auto& def : get_virtuals(inst->get_defs())) {
        blk.def.insert(def->get_virtual_id());
        all_vregs.insert(def->get_virtual_id());
      }
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = static_cast<int>(bbs.size()) - 1; i >= 0; --i) {
      auto old_in = info[i].live_in;
      auto old_out = info[i].live_out;

      // out = U succ.in
      std::unordered_set<int> new_out;
      for (int s : succs[i]) {
        new_out.insert(info[s].live_in.begin(), info[s].live_in.end());
      }

      // in = use U (out - def)
      std::unordered_set<int> new_in = info[i].use;
      for (int v : new_out) {
        if (!info[i].def.count(v)) new_in.insert(v);
      }

      if (new_in != info[i].live_in || new_out != info[i].live_out) {
        info[i].live_in.swap(new_in);
        info[i].live_out.swap(new_out);
        changed = true;
      }
    }
  }

  // 标记跨调用活跃的寄存器
  for (size_t i = 0; i < bbs.size(); ++i) {
    auto live = info[i].live_out;
    auto& insts = bbs[i]->get_instructions();
    for (auto it = insts.rbegin(); it != insts.rend(); ++it) {
      auto inst = *it;
      if (is_call_inst(inst)) {
        live_across_call.insert(live.begin(), live.end());
      }
      for (auto& def : get_virtuals(inst->get_defs())) {
        live.erase(def->get_virtual_id());
      }
      for (auto& use : get_virtuals(inst->get_uses())) {
        live.insert(use->get_virtual_id());
      }
    }
  }

  return info;
}

GraphColoringRegisterAllocator::AdjList
GraphColoringRegisterAllocator::build_interference(
    const std::vector<MipsBasicBlockPtr>& bbs,
    const std::vector<BlockLiveness>& info,
    const std::unordered_set<int>& /*live_across_call*/, int /*vreg_count*/,
    std::unordered_set<int>& nodes) const {
  AdjList graph;

  for (size_t i = 0; i < bbs.size(); ++i) {
    auto live = info[i].live_out;
    auto& insts = bbs[i]->get_instructions();
    for (auto it = insts.rbegin(); it != insts.rend(); ++it) {
      auto inst = *it;

      auto defs = get_virtuals(inst->get_defs());
      for (auto& def : defs) {
        int d = def->get_virtual_id();
        nodes.insert(d);
        for (int l : live) {
          if (l == d) continue;
          graph[d].insert(l);
          graph[l].insert(d);
        }
      }

      for (auto& def : defs) {
        live.erase(def->get_virtual_id());
      }
      for (auto& use : get_virtuals(inst->get_uses())) {
        live.insert(use->get_virtual_id());
        nodes.insert(use->get_virtual_id());
      }
    }
  }

  return graph;
}

std::pair<std::unordered_map<int, Reg>, std::unordered_set<int>>
GraphColoringRegisterAllocator::color_graph(
    const AdjList& graph, const std::unordered_set<int>& nodes,
    const std::unordered_set<int>& live_across_call) const {
  const auto colors = allocatable_regs();
  const int K = static_cast<int>(colors.size());

  std::unordered_map<int, int> degree;
  for (int n : nodes) {
    degree[n] = static_cast<int>(graph.count(n) ? graph.at(n).size() : 0);
  }

  std::vector<int> stack;
  std::unordered_set<int> spilled;
  std::unordered_set<int> remaining = nodes;

  while (!remaining.empty()) {
    auto it = std::find_if(remaining.begin(), remaining.end(),
                           [&](int n) { return degree[n] < K; });
    if (it != remaining.end()) {
      int n = *it;
      stack.push_back(n);
      remaining.erase(n);
      if (graph.count(n)) {
        for (int nb : graph.at(n)) {
          if (degree.count(nb) && degree[nb] > 0) degree[nb]--;
        }
      }
    } else {
      // spill candidate: highest degree
      int spill = *remaining.begin();
      for (int n : remaining) {
        if (degree[n] > degree[spill]) spill = n;
      }
      spilled.insert(spill);
      remaining.erase(spill);
      if (graph.count(spill)) {
        for (int nb : graph.at(spill)) {
          if (degree.count(nb) && degree[nb] > 0) degree[nb]--;
        }
      }
    }
  }

  std::unordered_map<int, Reg> assignment;

  while (!stack.empty()) {
    int n = stack.back();
    stack.pop_back();

    std::unordered_set<Reg> forbid;
    if (graph.count(n)) {
      for (int nb : graph.at(n)) {
        auto itc = assignment.find(nb);
        if (itc != assignment.end()) forbid.insert(itc->second);
      }
    }

    std::vector<Reg> candidates;
    if (live_across_call.count(n)) {
      for (Reg r : colors) {
        if (is_saved_reg(r)) candidates.push_back(r);
      }
      if (candidates.empty()) candidates = colors;
    } else {
      candidates = colors;
    }

    Reg chosen = Reg::VIRTUAL_BASE;
    for (Reg r : candidates) {
      if (!forbid.count(r)) {
        chosen = r;
        break;
      }
    }

    if (chosen == Reg::VIRTUAL_BASE) {
      spilled.insert(n);
    } else {
      assignment[n] = chosen;
    }
  }

  return {assignment, spilled};
}

bool GraphColoringRegisterAllocator::rewrite_spills(
    MipsFunctionPtr& func, const std::unordered_set<int>& spilled) {
  if (spilled.empty()) return true;

  std::unordered_map<int, int> spill_slot;
  for (int v : spilled) {
    spill_slot[v] = func->alloc_spill_slot();
  }

  bool changed = false;

  for (auto& bb : func->get_basic_blocks()) {
    auto& insts = bb->get_instructions();
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      auto inst = *it;

      std::vector<MipsInstPtr> insert_before;
      std::vector<MipsInstPtr> insert_after;
      std::unordered_map<int, std::shared_ptr<RegOperand>> use_repl;

      // 处理使用
      for (auto& use : get_virtuals(inst->get_uses())) {
        int id = use->get_virtual_id();
        if (!spilled.count(id)) continue;
        if (!use_repl.count(id)) {
          auto tmp = RegOperand::create_virtual(func->new_virtual_reg());
          use_repl[id] = tmp;
          insert_before.push_back(MemInst::create(MipsInstType::LW, tmp,
                                                  RegOperand::create(Reg::SP),
                                                  spill_slot[id]));
        }
        replace_operand(inst, id, use_repl[id]);
        changed = true;
      }

      // 处理定义
      for (auto& def : get_virtuals(inst->get_defs())) {
        int id = def->get_virtual_id();
        if (!spilled.count(id)) continue;
        auto tmp = RegOperand::create_virtual(func->new_virtual_reg());
        replace_operand(inst, id, tmp);
        insert_after.push_back(MemInst::create(MipsInstType::SW, tmp,
                                               RegOperand::create(Reg::SP),
                                               spill_slot[id]));
        changed = true;
      }

      for (auto& ni : insert_before) {
        insts.insert(it, ni);
      }

      auto next = std::next(it);
      for (auto& ni : insert_after) {
        insts.insert(next, ni);
      }
    }
  }

  return changed;
}

void GraphColoringRegisterAllocator::apply_colors(
    MipsFunctionPtr& func, const std::unordered_map<int, Reg>& colors) const {
  for (auto& bb : func->get_basic_blocks()) {
    for (auto& inst : bb->get_instructions()) {
      for (auto& [vreg, preg] : colors) {
        inst->replace_reg(vreg, preg);
      }
    }
  }
}

void GraphColoringRegisterAllocator::fix_stack_frame(
    MipsFunctionPtr& func, const StackFrame& old_frame,
    const StackFrame& new_frame) const {
  int size_diff = new_frame.total_size - old_frame.total_size;
  // 局部变量区起始偏移的差值 (saved_reg_size 变化导致)
  int local_var_offset_diff =
      new_frame.local_var_offset() - old_frame.local_var_offset();

  for (auto& bb : func->get_basic_blocks()) {
    for (auto& inst : bb->get_instructions()) {
      if (auto i_inst = std::dynamic_pointer_cast<ITypeInst>(inst)) {
        if (i_inst->get_type() == MipsInstType::ADDIU) {
          auto rt = i_inst->get_rt();
          auto rs = i_inst->get_rs();
          auto imm = i_inst->get_imm();
          // 调整栈指针修改指令 (函数序言/尾声)
          if (rt && rs && !rt->is_virtual() && !rs->is_virtual() &&
              rt->get_reg() == Reg::SP && rs->get_reg() == Reg::SP) {
            int imm_val = imm->get_value();
            if (imm_val == -old_frame.total_size) {
              i_inst->set_imm(ImmOperand::create(-new_frame.total_size));
            } else if (imm_val == old_frame.total_size) {
              i_inst->set_imm(ImmOperand::create(new_frame.total_size));
            }
          }
          // 调整 alloca 生成的地址计算指令: addiu $reg, $sp, offset
          // 这些指令计算局部变量的地址，偏移在 [old_local_var_offset,
          // old_total_size) 范围内
          else if (rs && !rs->is_virtual() && rs->get_reg() == Reg::SP) {
            int imm_val = imm->get_value();
            // 如果偏移在旧的局部变量区域内
            if (imm_val >= old_frame.local_var_offset() &&
                imm_val < old_frame.total_size) {
              i_inst->set_imm(
                  ImmOperand::create(imm_val + local_var_offset_diff));
            }
          }
        }
      }

      if (auto mem_inst = std::dynamic_pointer_cast<MemInst>(inst)) {
        auto mem = mem_inst->get_mem();
        auto base = mem->get_base();
        if (!base || base->is_virtual() || base->get_reg() != Reg::SP) continue;

        int offset = mem->get_offset();
        // 调整 ra 保存/恢复的偏移
        if (!mem_inst->get_defs().empty() &&
            mem_inst->get_defs()[0]->get_reg() == Reg::RA) {
          if (offset == old_frame.ra_offset())
            mem->set_offset(new_frame.ra_offset());
        } else if (mem_inst->get_type() == MipsInstType::SW &&
                   mem_inst->get_rt()->get_reg() == Reg::RA) {
          if (offset == old_frame.ra_offset())
            mem->set_offset(new_frame.ra_offset());
        }
        // 调整局部变量区域的 load/store 偏移
        else if (offset >= old_frame.local_var_offset() &&
                 offset < old_frame.total_size) {
          mem->set_offset(offset + local_var_offset_diff);
        }
        // 访问调用者栈参数的 lw 偏移调整
        else if (offset >= old_frame.total_size) {
          mem->set_offset(offset + size_diff);
        }
      }
    }
  }
}

bool GraphColoringRegisterAllocator::is_call_inst(
    const MipsInstPtr& inst) const {
  auto type = inst->get_type();
  return type == MipsInstType::JAL || type == MipsInstType::JALR ||
         type == MipsInstType::SYSCALL;
}

std::vector<std::shared_ptr<RegOperand>>
GraphColoringRegisterAllocator::get_virtuals(
    const std::vector<std::shared_ptr<RegOperand>>& ops) {
  std::vector<std::shared_ptr<RegOperand>> res;
  for (auto& op : ops) {
    if (op && op->is_virtual()) res.push_back(op);
  }
  return res;
}

bool GraphColoringRegisterAllocator::replace_operand(
    const MipsInstPtr& inst, int old_vreg,
    const std::shared_ptr<RegOperand>& replacement) {
  bool replaced = false;

  if (auto r = std::dynamic_pointer_cast<RTypeInst>(inst)) {
    if (r->get_rd() && r->get_rd()->is_virtual() &&
        r->get_rd()->get_virtual_id() == old_vreg) {
      r->set_rd(replacement);
      replaced = true;
    }
    if (r->get_rs() && r->get_rs()->is_virtual() &&
        r->get_rs()->get_virtual_id() == old_vreg) {
      r->set_rs(replacement);
      replaced = true;
    }
    if (r->get_rt() && r->get_rt()->is_virtual() &&
        r->get_rt()->get_virtual_id() == old_vreg) {
      r->set_rt(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto s = std::dynamic_pointer_cast<ShiftInst>(inst)) {
    if (s->get_rd() && s->get_rd()->is_virtual() &&
        s->get_rd()->get_virtual_id() == old_vreg) {
      s->set_rd(replacement);
      replaced = true;
    }
    if (s->get_rt() && s->get_rt()->is_virtual() &&
        s->get_rt()->get_virtual_id() == old_vreg) {
      s->set_rt(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto it = std::dynamic_pointer_cast<ITypeInst>(inst)) {
    if (it->get_rt() && it->get_rt()->is_virtual() &&
        it->get_rt()->get_virtual_id() == old_vreg) {
      it->set_rt(replacement);
      replaced = true;
    }
    if (it->get_rs() && it->get_rs()->is_virtual() &&
        it->get_rs()->get_virtual_id() == old_vreg) {
      it->set_rs(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto li = std::dynamic_pointer_cast<LuiInst>(inst)) {
    if (li->get_defs().size() && li->get_defs()[0]->is_virtual() &&
        li->get_defs()[0]->get_virtual_id() == old_vreg) {
      // LuiInst only defines rt
      li->set_rt(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto mem = std::dynamic_pointer_cast<MemInst>(inst)) {
    if (mem->get_rt() && mem->get_rt()->is_virtual() &&
        mem->get_rt()->get_virtual_id() == old_vreg) {
      mem->set_rt(replacement);
      replaced = true;
    }
    auto base = mem->get_mem()->get_base();
    if (base && base->is_virtual() && base->get_virtual_id() == old_vreg) {
      mem->get_mem()->set_base(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto br = std::dynamic_pointer_cast<BranchInst>(inst)) {
    if (br->get_rs() && br->get_rs()->is_virtual() &&
        br->get_rs()->get_virtual_id() == old_vreg) {
      br->set_rs(replacement);
      replaced = true;
    }
    if (br->get_rt() && br->get_rt()->is_virtual() &&
        br->get_rt()->get_virtual_id() == old_vreg) {
      br->set_rt(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto brz = std::dynamic_pointer_cast<BranchZeroInst>(inst)) {
    if (brz->get_rs() && brz->get_rs()->is_virtual() &&
        brz->get_rs()->get_virtual_id() == old_vreg) {
      brz->set_rs(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto div = std::dynamic_pointer_cast<DivInst>(inst)) {
    if (div->get_rs() && div->get_rs()->is_virtual() &&
        div->get_rs()->get_virtual_id() == old_vreg) {
      div->set_rs(replacement);
      replaced = true;
    }
    if (div->get_rt() && div->get_rt()->is_virtual() &&
        div->get_rt()->get_virtual_id() == old_vreg) {
      div->set_rt(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto mult = std::dynamic_pointer_cast<MultInst>(inst)) {
    if (mult->get_rs() && mult->get_rs()->is_virtual() &&
        mult->get_rs()->get_virtual_id() == old_vreg) {
      mult->set_rs(replacement);
      replaced = true;
    }
    if (mult->get_rt() && mult->get_rt()->is_virtual() &&
        mult->get_rt()->get_virtual_id() == old_vreg) {
      mult->set_rt(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto mf = std::dynamic_pointer_cast<MfInst>(inst)) {
    if (mf->get_rd() && mf->get_rd()->is_virtual() &&
        mf->get_rd()->get_virtual_id() == old_vreg) {
      mf->set_rd(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto la = std::dynamic_pointer_cast<LaInst>(inst)) {
    if (la->get_rd() && la->get_rd()->is_virtual() &&
        la->get_rd()->get_virtual_id() == old_vreg) {
      la->set_rd(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto li_inst = std::dynamic_pointer_cast<LiInst>(inst)) {
    if (li_inst->get_rd() && li_inst->get_rd()->is_virtual() &&
        li_inst->get_rd()->get_virtual_id() == old_vreg) {
      li_inst->set_rd(replacement);
      replaced = true;
    }
    return replaced;
  }

  if (auto mv = std::dynamic_pointer_cast<MoveInst>(inst)) {
    if (mv->get_rd() && mv->get_rd()->is_virtual() &&
        mv->get_rd()->get_virtual_id() == old_vreg) {
      mv->set_rd(replacement);
      replaced = true;
    }
    if (mv->get_rs() && mv->get_rs()->is_virtual() &&
        mv->get_rs()->get_virtual_id() == old_vreg) {
      mv->set_rs(replacement);
      replaced = true;
    }
    return replaced;
  }

  return replaced;
}

void GraphColoringRegisterAllocator::insert_saved_reg_saves(
    MipsFunctionPtr& func) const {
  const auto& frame = func->get_stack_frame();
  if (frame.saved_reg_size == 0) return;

  auto saved_regs = func->get_saved_regs();
  if (saved_regs.empty()) return;

  auto entry = func->get_basic_blocks().front();
  auto& insts = entry->get_instructions();

  // 已存在则不重复插入
  bool exists = true;
  int check_offset = frame.saved_reg_offset();
  for (Reg r : saved_regs) {
    bool found = false;
    for (auto& inst : insts) {
      if (auto mem = std::dynamic_pointer_cast<MemInst>(inst)) {
        if (mem->get_type() == MipsInstType::SW &&
            !mem->get_rt()->is_virtual() && mem->get_rt()->get_reg() == r) {
          auto base = mem->get_mem()->get_base();
          if (base && !base->is_virtual() && base->get_reg() == Reg::SP &&
              mem->get_mem()->get_offset() == check_offset) {
            found = true;
            break;
          }
        }
      }
    }
    if (!found) {
      exists = false;
      break;
    }
    check_offset += 4;
  }
  if (exists) return;

  auto pos = insts.begin();

  // 找到栈指针调整指令，插入在其后
  for (auto it = insts.begin(); it != insts.end(); ++it) {
    if (auto add = std::dynamic_pointer_cast<ITypeInst>(*it)) {
      if (add->get_type() == MipsInstType::ADDIU) {
        auto rt = add->get_rt();
        auto rs = add->get_rs();
        auto imm = add->get_imm();
        if (rt && rs && !rt->is_virtual() && !rs->is_virtual() &&
            rt->get_reg() == Reg::SP && rs->get_reg() == Reg::SP &&
            imm->get_value() == -frame.total_size) {
          pos = std::next(it);
          break;
        }
      }
    }
  }

  // 如果有保存 $ra，跳过它，在其后插入 callee-saved 保存
  if (pos != insts.end()) {
    auto next_it = pos;
    if (next_it != insts.end()) {
      if (auto mem = std::dynamic_pointer_cast<MemInst>(*next_it)) {
        if (mem->get_type() == MipsInstType::SW &&
            mem->get_rt()->get_reg() == Reg::RA) {
          pos = std::next(next_it);
        }
      }
    }
  }

  int offset = frame.saved_reg_offset();
  for (Reg r : saved_regs) {
    insts.insert(pos, MemInst::create(MipsInstType::SW, RegOperand::create(r),
                                      RegOperand::create(Reg::SP), offset));
    offset += 4;
  }
}

void GraphColoringRegisterAllocator::insert_saved_reg_restores(
    MipsFunctionPtr& func) const {
  const auto& frame = func->get_stack_frame();
  if (frame.saved_reg_size == 0) return;

  auto saved_regs = func->get_saved_regs();
  if (saved_regs.empty()) return;

  for (auto& bb : func->get_basic_blocks()) {
    auto& insts = bb->get_instructions();

    // 如果已存在恢复指令则跳过当前块
    bool exists = false;
    int check_offset = frame.saved_reg_offset();
    for (auto& inst : insts) {
      if (auto mem = std::dynamic_pointer_cast<MemInst>(inst)) {
        if (mem->get_type() == MipsInstType::LW) {
          auto base = mem->get_mem()->get_base();
          if (base && !base->is_virtual() && base->get_reg() == Reg::SP &&
              mem->get_mem()->get_offset() == check_offset) {
            exists = true;
            break;
          }
        }
      }
    }
    if (exists) continue;

    for (auto it = insts.begin(); it != insts.end(); ++it) {
      if (auto add = std::dynamic_pointer_cast<ITypeInst>(*it)) {
        if (add->get_type() == MipsInstType::ADDIU) {
          auto rt = add->get_rt();
          auto rs = add->get_rs();
          auto imm = add->get_imm();
          if (rt && rs && !rt->is_virtual() && !rs->is_virtual() &&
              rt->get_reg() == Reg::SP && rs->get_reg() == Reg::SP &&
              imm->get_value() == frame.total_size) {
            // 在恢复栈指针前恢复保存寄存器
            int offset = frame.saved_reg_offset();
            for (Reg r : saved_regs) {
              insts.insert(
                  it, MemInst::create(MipsInstType::LW, RegOperand::create(r),
                                      RegOperand::create(Reg::SP), offset));
              offset += 4;
            }
            break;
          }
        }
      }
    }
  }
}

}  // namespace backend::mips
