#include "midend/opt/engine.hpp"

#include <vector>

#include "midend/opt/pass.hpp"

namespace midend::opt {
namespace {
using namespace pass;
using PassPipeline = std::vector<Pass>;

auto build_pipeline() -> PassPipeline {
  PassPipeline pipeline = {
      // Build SSA form.
      RemoveUnreachableInstructions{},
      ControlFlowGraphAnalysis{},
      RemoveUnreachableBlocks{},
      RemoveUnreachableInstructions{},
      ControlFlowGraphAnalysis{},
      DominanceAnalysis{},
      Mem2Reg{},
  };

  // Repeat scalar and control-flow simplification four times.
  for (int i = 0; i < 4; ++i) {
    pipeline.emplace_back(ConstantFolding{});
    pipeline.emplace_back(DeadCodeElimination{});
    pipeline.emplace_back(PhiElimination{});
    pipeline.emplace_back(RemoveUnreachableInstructions{});
    pipeline.emplace_back(ControlFlowGraphAnalysis{});
    pipeline.emplace_back(SimplifyCFG{});
    pipeline.emplace_back(RemoveUnreachableInstructions{});
    pipeline.emplace_back(ControlFlowGraphAnalysis{});
    pipeline.emplace_back(BlockMerge{});
  }

  return pipeline;
}

}  // namespace

void optimize(llvm::Module& module) {
  for (const auto& pass : build_pipeline()) {
    pass.run(module);
  }
}

}  // namespace midend::opt
