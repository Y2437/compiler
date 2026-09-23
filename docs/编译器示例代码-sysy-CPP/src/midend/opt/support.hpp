#ifndef BUAA_COMPILER_OPT_SUPPORT_HPP
#define BUAA_COMPILER_OPT_SUPPORT_HPP

#include <algorithm>
#include <memory>
#include <vector>

namespace midend::llvm {
class BasicBlock;
}

namespace midend::opt {

struct BasicBlockOptInfo {
  std::vector<std::weak_ptr<llvm::BasicBlock>> predecessors;
  std::vector<std::weak_ptr<llvm::BasicBlock>> successors;
  std::weak_ptr<llvm::BasicBlock> idom;
  std::vector<std::weak_ptr<llvm::BasicBlock>> immediate_dominated;
  std::vector<std::weak_ptr<llvm::BasicBlock>> dominance_frontier;

  void remove_predecessor(const std::shared_ptr<llvm::BasicBlock>& block) {
    erase_block(predecessors, block);
  }

  void remove_successor(const std::shared_ptr<llvm::BasicBlock>& block) {
    erase_block(successors, block);
  }

 private:
  static void erase_block(std::vector<std::weak_ptr<llvm::BasicBlock>>& blocks,
                          const std::shared_ptr<llvm::BasicBlock>& target) {
    blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                                [&target](const auto& block) {
                                  return block.lock() == target;
                                }),
                 blocks.end());
  }
};

}  // namespace midend::opt

#endif
