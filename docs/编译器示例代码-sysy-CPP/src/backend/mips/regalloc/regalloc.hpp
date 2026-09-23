#ifndef BUAA_COMPILER_MIPS_REGALLOC
#define BUAA_COMPILER_MIPS_REGALLOC

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "backend/mips/ir/function.hpp"
#include "backend/mips/ir/instruction.hpp"
#include "backend/mips/ir/module.hpp"

namespace backend::mips {
class RegisterAllocator {
 public:
  virtual ~RegisterAllocator() = default;
  virtual void allocate(MipsModule& module) = 0;
};

// The teaching allocator spills every virtual register to the stack.
class SimpleRegisterAllocator : public RegisterAllocator {
 public:
  void allocate(MipsModule& module) override;

 private:
  void allocate_function(MipsFunctionPtr& func);
  int get_stack_slot(int vreg_id);
  void rewrite_instruction(MipsBasicBlockPtr& bb);
  void load_vreg(std::list<MipsInstPtr>& insts,
                 std::list<MipsInstPtr>::iterator& pos,
                 std::shared_ptr<RegOperand> vreg, Reg target);
  void store_vreg(std::list<MipsInstPtr>& insts,
                  std::list<MipsInstPtr>::iterator& pos,
                  std::shared_ptr<RegOperand> vreg, Reg source);
  void fix_stack_frame_size(MipsFunctionPtr& func, int old_size, int new_size);

  std::unordered_map<int, int> vreg_to_stack_;
  MipsFunctionPtr current_func_;
};

class GraphColoringRegisterAllocator : public RegisterAllocator {
 public:
  void allocate(MipsModule& module) override;

 private:
  struct BlockLiveness {
    std::unordered_set<int> use;
    std::unordered_set<int> def;
    std::unordered_set<int> live_in;
    std::unordered_set<int> live_out;
  };

  using AdjList = std::unordered_map<int, std::unordered_set<int>>;

  void allocate_function(MipsFunctionPtr& func);
  auto collect_blocks(const MipsFunctionPtr& func) const
      -> std::vector<MipsBasicBlockPtr>;
  auto build_label_map(const std::vector<MipsBasicBlockPtr>& blocks) const
      -> std::unordered_map<std::string, MipsBasicBlockPtr>;
  auto build_successors(
      const std::vector<MipsBasicBlockPtr>& blocks,
      const std::unordered_map<std::string, MipsBasicBlockPtr>& label_map) const
      -> std::vector<std::vector<int>>;
  auto compute_liveness(const std::vector<MipsBasicBlockPtr>& blocks,
                        const std::vector<std::vector<int>>& successors,
                        std::unordered_set<int>& all_vregs,
                        std::unordered_set<int>& live_across_call) const
      -> std::vector<BlockLiveness>;
  auto build_interference(const std::vector<MipsBasicBlockPtr>& blocks,
                          const std::vector<BlockLiveness>& liveness,
                          const std::unordered_set<int>& live_across_call,
                          int vreg_count, std::unordered_set<int>& nodes) const
      -> AdjList;
  auto color_graph(const AdjList& graph, const std::unordered_set<int>& nodes,
                   const std::unordered_set<int>& live_across_call) const
      -> std::pair<std::unordered_map<int, Reg>, std::unordered_set<int>>;
  bool rewrite_spills(MipsFunctionPtr& func,
                      const std::unordered_set<int>& spilled);
  void apply_colors(MipsFunctionPtr& func,
                    const std::unordered_map<int, Reg>& colors) const;
  void fix_stack_frame(MipsFunctionPtr& func, const StackFrame& old_frame,
                       const StackFrame& new_frame) const;
  void insert_saved_reg_saves(MipsFunctionPtr& func) const;
  void insert_saved_reg_restores(MipsFunctionPtr& func) const;
  bool is_call_inst(const MipsInstPtr& inst) const;
  static auto get_virtuals(
      const std::vector<std::shared_ptr<RegOperand>>& operands)
      -> std::vector<std::shared_ptr<RegOperand>>;
  static bool replace_operand(const MipsInstPtr& inst, int old_vreg,
                              const std::shared_ptr<RegOperand>& replacement);
};

}  // namespace backend::mips

#endif
