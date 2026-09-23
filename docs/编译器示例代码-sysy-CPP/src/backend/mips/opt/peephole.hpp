#ifndef BUAA_COMPILER_MIPS_PEEPHOLE
#define BUAA_COMPILER_MIPS_PEEPHOLE

#include <memory>

#include "backend/mips/ir/function.hpp"
#include "backend/mips/ir/instruction.hpp"
#include "backend/mips/ir/module.hpp"

namespace backend::mips {

class PeepholeOptimizer {
 public:
  explicit PeepholeOptimizer(MipsModule& module) : module_(module) {}

  void run_before_ra();

 private:
  MipsModule& module_;

  bool remove_same_reg_move(MipsBasicBlockPtr& bb);
  bool optimize_addiu_zero(MipsBasicBlockPtr& bb);
  bool optimize_li_zero(MipsBasicBlockPtr& bb);
  bool optimize_add_zero(MipsBasicBlockPtr& bb);
  bool remove_useless_load_store(MipsBasicBlockPtr& bb);
  bool optimize_store_load(MipsBasicBlockPtr& bb);
  bool reuse_constant_value(MipsBasicBlockPtr& bb);
  bool optimize_mul_power_of_two(MipsBasicBlockPtr& bb);
  bool same_memory_location(const std::shared_ptr<MemOperand>& a,
                            const std::shared_ptr<MemOperand>& b);
  static bool is_power_of_two(int n) { return n > 0 && (n & (n - 1)) == 0; }
  static int log2_int(int n) {
    int result = 0;
    while (n > 1) {
      n >>= 1;
      result++;
    }
    return result;
  }
};

}  // namespace backend::mips

#endif
