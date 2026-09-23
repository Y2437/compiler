#ifndef BUAA_COMPILER_MIPS_CFG_OPT
#define BUAA_COMPILER_MIPS_CFG_OPT

#include <string>
#include <unordered_set>

#include "backend/mips/ir/function.hpp"
#include "backend/mips/ir/instruction.hpp"
#include "backend/mips/ir/module.hpp"

namespace backend::mips {

class CFGOptimizer {
 public:
  explicit CFGOptimizer(MipsModule& module) : module_(module) {}

  void run();

 private:
  bool redirect_goto(MipsFunctionPtr& function);
  bool remove_unreachable_blocks(MipsFunctionPtr& function);
  bool remove_empty_blocks(MipsFunctionPtr& function);

  auto get_jump_target(const MipsBasicBlockPtr& block) -> std::string;
  auto find_block_by_label(MipsFunctionPtr& function, const std::string& label)
      -> MipsBasicBlockPtr;
  bool is_jump_only_block(const MipsBasicBlockPtr& block);
  void update_jump_targets(MipsFunctionPtr& function,
                           const std::string& old_label,
                           const std::string& new_label);
  auto get_reachable_blocks(MipsFunctionPtr& function)
      -> std::unordered_set<MipsBasicBlockPtr>;

  MipsModule& module_;
};

}  // namespace backend::mips

#endif
