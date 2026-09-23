#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "midend/opt/opt_util.hpp"
#include "midend/opt/pass.hpp"
#include "midend/opt/support.hpp"

namespace midend::opt::pass {
namespace {

class LengauerTarjan {
 public:
  using BlockPtr = std::shared_ptr<llvm::BasicBlock>;

  void compute(const std::shared_ptr<llvm::Function>& func) {
    const auto& blocks = func->get_basic_blocks();
    if (blocks.empty()) return;

    init(func);
    dfs(blocks.front());

    if (n_ <= 1) {
      return;
    }

    // Variable names follow the standard Lengauer-Tarjan presentation.
    for (int i = n_ - 1; i >= 1; --i) {
      auto w = vertex_[i];

      for (const auto& weak_pred : w->opt_info.predecessors) {
        if (auto v = weak_pred.lock()) {
          auto dfn = dfn_.find(v);
          if (dfn == dfn_.end() || dfn->second == -1) continue;
          auto u = eval(v);
          if (semi_[u] < semi_[w]) {
            semi_[w] = semi_[u];
          }
        }
      }

      bucket_[vertex_[semi_[w]]].push_back(w);
      link(parent_[w], w);

      for (auto v : bucket_[parent_[w]]) {
        auto u = eval(v);
        idom_[v] = (semi_[u] < semi_[v]) ? u : parent_[w];
      }
      bucket_[parent_[w]].clear();
    }

    for (int i = 1; i < n_; ++i) {
      auto w = vertex_[i];
      if (idom_[w] != vertex_[semi_[w]]) {
        idom_[w] = idom_[idom_[w]];
      }
    }

    for (const auto& block : blocks) {
      auto iter = idom_.find(block);
      if (iter != idom_.end() && iter->second) {
        block->opt_info.idom = iter->second;
        iter->second->opt_info.immediate_dominated.push_back(block);
      }
    }

    compute_dominance_frontier(func);
  }

 private:
  void init(const std::shared_ptr<llvm::Function>& func) {
    n_ = 0;
    vertex_.clear();
    dfn_.clear();
    semi_.clear();
    idom_.clear();
    parent_.clear();
    ancestor_.clear();
    label_.clear();
    bucket_.clear();

    for (const auto& block : func->get_basic_blocks()) {
      block->opt_info.idom.reset();
      block->opt_info.immediate_dominated.clear();
      block->opt_info.dominance_frontier.clear();

      dfn_[block] = -1;
      label_[block] = block;
    }
  }

  void dfs(BlockPtr v) {
    dfn_[v] = n_;
    semi_[v] = n_;
    vertex_.push_back(v);
    n_++;

    for (const auto& weak_succ : v->opt_info.successors) {
      if (auto w = weak_succ.lock()) {
        if (dfn_.find(w) == dfn_.end() || dfn_[w] == -1) {
          parent_[w] = v;
          dfs(w);
        }
      }
    }
  }

  void link(BlockPtr v, BlockPtr w) { ancestor_[w] = v; }

  BlockPtr eval(BlockPtr v) {
    if (ancestor_.find(v) == ancestor_.end() || !ancestor_[v]) {
      return v;
    }
    compress(v);
    return label_[v];
  }

  void compress(BlockPtr v) {
    if (ancestor_.find(ancestor_[v]) != ancestor_.end() &&
        ancestor_[ancestor_[v]]) {
      compress(ancestor_[v]);
      if (semi_[label_[ancestor_[v]]] < semi_[label_[v]]) {
        label_[v] = label_[ancestor_[v]];
      }
      ancestor_[v] = ancestor_[ancestor_[v]];
    }
  }

  void compute_dominance_frontier(const std::shared_ptr<llvm::Function>& func) {
    for (const auto& block : func->get_basic_blocks()) {
      const auto& preds = block->opt_info.predecessors;
      if (preds.size() >= 2) {
        for (const auto& weak_pred : preds) {
          if (auto runner = weak_pred.lock()) {
            auto idom_of_block = idom_.count(block) ? idom_[block] : nullptr;
            while (runner && runner != idom_of_block) {
              const auto& frontier = runner->opt_info.dominance_frontier;
              if (std::none_of(frontier.begin(), frontier.end(),
                               [&block](const auto& item) {
                                 return item.lock() == block;
                               })) {
                runner->opt_info.dominance_frontier.push_back(block);
              }

              if (idom_.find(runner) != idom_.end()) {
                runner = idom_[runner];
              } else {
                break;
              }
            }
          }
        }
      }
    }
  }

 private:
  int n_ = 0;
  std::vector<BlockPtr> vertex_;
  std::unordered_map<BlockPtr, int> dfn_;
  std::unordered_map<BlockPtr, int> semi_;
  std::unordered_map<BlockPtr, BlockPtr> idom_;
  std::unordered_map<BlockPtr, BlockPtr> parent_;
  std::unordered_map<BlockPtr, BlockPtr> ancestor_;
  std::unordered_map<BlockPtr, BlockPtr> label_;
  std::unordered_map<BlockPtr, std::vector<BlockPtr>> bucket_;
};

}  // namespace

bool DominanceAnalysis::run(llvm::Module& module) {
  LengauerTarjan lt;

  for (const auto& func : module.get_functions()) {
    lt.compute(func);
  }

  return false;
}

}  // namespace midend::opt::pass
