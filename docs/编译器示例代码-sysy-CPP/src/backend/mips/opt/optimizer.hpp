#ifndef BUAA_COMPILER_MIPS_OPTIMIZER
#define BUAA_COMPILER_MIPS_OPTIMIZER

#include "backend/mips/ir/module.hpp"
#include "backend/mips/opt/cfg_opt.hpp"
#include "backend/mips/opt/dce.hpp"
#include "backend/mips/opt/peephole.hpp"

namespace backend::mips {

class MipsOptimizer {
 public:
  explicit MipsOptimizer(MipsModule& module) : module_(module) {}

  void run_before_ra() {
    CFGOptimizer cfg(module_);
    cfg.run();
    DeadCodeEliminator{module_}.run();
    PeepholeOptimizer{module_}.run_before_ra();
    cfg.run();
  }

 private:
  MipsModule& module_;
};

}  // namespace backend::mips

#endif
