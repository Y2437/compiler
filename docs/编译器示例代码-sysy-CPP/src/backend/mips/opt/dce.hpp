#ifndef BUAA_COMPILER_MIPS_DCE
#define BUAA_COMPILER_MIPS_DCE

#include <unordered_map>
#include <unordered_set>

#include "backend/mips/ir/function.hpp"
#include "backend/mips/ir/instruction.hpp"
#include "backend/mips/ir/module.hpp"

namespace backend::mips {

struct LivenessInfo {
  std::unordered_set<int> live_in;
  std::unordered_set<int> live_out;
  std::unordered_set<int> live_def;
  std::unordered_set<int> live_use;
};

class DeadCodeEliminator {
 public:
  explicit DeadCodeEliminator(MipsModule& module) : module_(module) {}

  void run();

 private:
  void compute_liveness(MipsFunctionPtr& func);
  void build_cfg(MipsFunctionPtr& func);
  bool eliminate_dead_instructions(MipsFunctionPtr& func);
  bool can_delete_instruction(const MipsInstPtr& inst);
  bool has_side_effects(const MipsInstPtr& inst);
  auto get_uses(const MipsInstPtr& inst) -> std::unordered_set<int>;
  auto get_defs(const MipsInstPtr& inst) -> std::unordered_set<int>;

  MipsModule& module_;
  std::unordered_map<MipsBasicBlockPtr, LivenessInfo> liveness_map_;
  std::unordered_map<MipsBasicBlockPtr, std::vector<MipsBasicBlockPtr>>
      successors_;
};

}  // namespace backend::mips

#endif
