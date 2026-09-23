#ifndef BUAA_COMPILER_MIPS_FUNCTION
#define BUAA_COMPILER_MIPS_FUNCTION

#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "backend/mips/ir/instruction.hpp"

namespace backend::mips {

class MipsBasicBlock;
class MipsFunction;
using MipsBasicBlockPtr = std::shared_ptr<MipsBasicBlock>;
using MipsFunctionPtr = std::shared_ptr<MipsFunction>;

class MipsBasicBlock {
 public:
  explicit MipsBasicBlock(std::string label) : label_(std::move(label)) {}

  void add_inst(const MipsInstPtr& inst) { instructions_.push_back(inst); }
  void add_inst_front(const MipsInstPtr& inst) {
    instructions_.push_front(inst);
  }

  void insert_inst(std::list<MipsInstPtr>::iterator pos,
                   const MipsInstPtr& inst) {
    instructions_.insert(pos, inst);
  }

  std::list<MipsInstPtr>& get_instructions() { return instructions_; }
  const std::list<MipsInstPtr>& get_instructions() const {
    return instructions_;
  }

  const std::string& get_label() const { return label_; }
  void set_label(const std::string& label) { label_ = label; }

  std::string to_string() const {
    std::string result = label_ + ":\n";
    for (const auto& inst : instructions_) {
      result += "\t" + inst->to_string() + "\n";
    }
    return result;
  }

  static MipsBasicBlockPtr create(const std::string& label) {
    return std::make_shared<MipsBasicBlock>(label);
  }

 private:
  std::string label_;
  std::list<MipsInstPtr> instructions_;
};

/*
 * MIPS O32 ABI Stack Frame Layout (高地址 -> 低地址):
 *
 * +-------------------------+ <- caller's $sp (进入函数前)
 * |   caller stack frame    |
 * +-------------------------+ <- 当前函数的 $fp (如果使用)
 * |   padding (8B align)    |
 * +-------------------------+
 * |    local variables      | <- local_var_offset() 起始
 * +-------------------------+
 * |         $ra             | <- ra_offset()
 * +-------------------------+
 * | callee-saved regs       | <- saved_reg_offset() 起始
 * | ($s0-$s7, $fp if used)  |
 * +-------------------------+
 * |   arg4, arg5, ...       | <- 超过4个的参数 (由caller放置)
 * +-------------------------+
 * |   arg3 ($a3 shadow)     | <- 12($sp)
 * |   arg2 ($a2 shadow)     | <- 8($sp)
 * |   arg1 ($a1 shadow)     | <- 4($sp)
 * |   arg0 ($a0 shadow)     | <- 0($sp)  参数构造区
 * +-------------------------+ <- 当前 $sp
 *
 */
struct StackFrame {
  int total_size = 0;
  int local_var_size = 0;
  int saved_reg_size = 0;
  int arg_build_size = 16;
  bool need_save_ra = false;

  int ra_size() const { return need_save_ra ? 4 : 0; }

  int saved_reg_offset() const { return arg_build_size; }
  int ra_offset() const { return arg_build_size + saved_reg_size; }
  int local_var_offset() const {
    return arg_build_size + saved_reg_size + ra_size();
  }
};

class MipsFunction {
 public:
  explicit MipsFunction(std::string name) : name_(std::move(name)) {}

  const std::string& get_name() const { return name_; }

  void add_basic_block(const MipsBasicBlockPtr& bb) {
    basic_blocks_.push_back(bb);
  }
  std::list<MipsBasicBlockPtr>& get_basic_blocks() { return basic_blocks_; }
  const std::list<MipsBasicBlockPtr>& get_basic_blocks() const {
    return basic_blocks_;
  }

  StackFrame& get_stack_frame() { return stack_frame_; }
  const StackFrame& get_stack_frame() const { return stack_frame_; }

  int alloc_stack_slot(int size, int align = 4) {
    stack_frame_.local_var_size =
        (stack_frame_.local_var_size + align - 1) / align * align;
    int offset = stack_frame_.local_var_offset() + stack_frame_.local_var_size;
    stack_frame_.local_var_size += size;
    return offset;
  }

  int alloc_spill_slot() { return alloc_stack_slot(4, 4); }

  int new_virtual_reg() { return next_virtual_reg_++; }
  int get_virtual_reg_count() const { return next_virtual_reg_; }

  void set_param_count(int count) { param_count_ = count; }
  int get_param_count() const { return param_count_; }

  void finalize_stack_frame() {
    stack_frame_.saved_reg_size = static_cast<int>(saved_regs_.size()) * 4;

    stack_frame_.total_size =
        stack_frame_.arg_build_size + stack_frame_.saved_reg_size +
        stack_frame_.ra_size() + stack_frame_.local_var_size;

    stack_frame_.total_size = (stack_frame_.total_size + 7) / 8 * 8;
  }

  void add_saved_reg(Reg reg) { saved_regs_.insert(reg); }
  const std::set<Reg>& get_saved_regs() const { return saved_regs_; }

  void update_max_call_args(int args) {
    if (args > 4) {
      int needed = (args - 4) * 4 + 16;
      if (needed > stack_frame_.arg_build_size) {
        stack_frame_.arg_build_size = needed;
      }
    }
  }

  std::string to_string() const {
    std::string result;
    for (const auto& bb : basic_blocks_) {
      result += bb->to_string();
    }
    return result;
  }

  static MipsFunctionPtr create(const std::string& name) {
    return std::make_shared<MipsFunction>(name);
  }

 private:
  std::string name_;
  std::list<MipsBasicBlockPtr> basic_blocks_;
  StackFrame stack_frame_;
  int next_virtual_reg_ = 0;
  int param_count_ = 0;
  std::set<Reg> saved_regs_;
};

}  // namespace backend::mips

#endif
