#ifndef BUAA_COMPILER_OPT_PASS_HPP
#define BUAA_COMPILER_OPT_PASS_HPP

#include <memory>
#include <type_traits>
#include <utility>

#include "midend/llvm/module.hpp"

namespace midend::opt::pass {

class PassBase {
 public:
  virtual ~PassBase() = default;
  virtual bool run(llvm::Module& module) = 0;
  virtual bool run_to_fixpoint() const { return false; }
};

class Pass {
 public:
  template <typename PassType,
            std::enable_if_t<
                std::is_base_of_v<PassBase, std::decay_t<PassType>>, int> = 0>
  Pass(PassType&& pass)
      : pass_(std::make_shared<std::decay_t<PassType>>(
            std::forward<PassType>(pass))) {}

  bool run(llvm::Module& module) const {
    bool modified = false;
    bool changed = false;
    do {
      changed = pass_->run(module);
      modified |= changed;
    } while (changed && pass_->run_to_fixpoint());
    return modified;
  }

 private:
  std::shared_ptr<PassBase> pass_;
};

#define DECLARE_PASS(name)                   \
  class name final : public PassBase {       \
   public:                                   \
    bool run(llvm::Module& module) override; \
  }

#define DECLARE_FIXPOINT_PASS(name)                        \
  class name final : public PassBase {                     \
   public:                                                 \
    bool run(llvm::Module& module) override;               \
    bool run_to_fixpoint() const override { return true; } \
  }

DECLARE_PASS(ControlFlowGraphAnalysis);
DECLARE_PASS(DominanceAnalysis);
DECLARE_PASS(RemoveUnreachableInstructions);
DECLARE_PASS(RemoveUnreachableBlocks);
DECLARE_PASS(ConstantFolding);
DECLARE_FIXPOINT_PASS(DeadCodeElimination);
DECLARE_PASS(Mem2Reg);
DECLARE_PASS(BlockMerge);
DECLARE_FIXPOINT_PASS(SimplifyCFG);
DECLARE_FIXPOINT_PASS(PhiElimination);
DECLARE_PASS(RemovePhi);

#undef DECLARE_FIXPOINT_PASS
#undef DECLARE_PASS

}  // namespace midend::opt::pass

#endif
