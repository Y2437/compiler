#ifndef BUAA_COMPILER_OPT_ENGINE_HPP
#define BUAA_COMPILER_OPT_ENGINE_HPP

#include "midend/llvm/module.hpp"

namespace midend::opt {

void optimize(llvm::Module& module);

}  // namespace midend::opt

#endif
