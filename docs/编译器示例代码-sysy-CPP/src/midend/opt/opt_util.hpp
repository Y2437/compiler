#ifndef BUAA_COMPILER_OPT_UTIL_HPP
#define BUAA_COMPILER_OPT_UTIL_HPP

#include <algorithm>
#include <memory>
#include <string>

#include "midend/llvm/value.hpp"

namespace midend::llvm {
class Phi;
}

namespace midend::opt::util {

inline auto gen_temp_name() -> std::string {
  static int counter = 0;
  return "tmp_" + std::to_string(counter++);
}

inline auto gen_block_name() -> std::string {
  static int counter = 0;
  return "opt_block_" + std::to_string(counter++);
}

void replace_all_uses_with(const std::shared_ptr<llvm::Value>& old_value,
                           const std::shared_ptr<llvm::Value>& new_value);
void remove_instruction(const std::shared_ptr<llvm::Instruction>& instruction);
void remove_all_operands(const std::shared_ptr<llvm::User>& user);
void remove_instruction_from_parent(
    const std::shared_ptr<llvm::Instruction>& instruction);
void substitute_operand(const std::shared_ptr<llvm::User>& user,
                        const std::shared_ptr<llvm::Value>& old_operand,
                        const std::shared_ptr<llvm::Value>& new_operand);
auto get_phi_value(const std::shared_ptr<llvm::Phi>& phi,
                   const std::shared_ptr<llvm::BasicBlock>& block)
    -> std::shared_ptr<llvm::Value>;
bool is_terminator(const std::shared_ptr<llvm::Instruction>& instruction);

template <typename Container, typename T>
bool contains(const Container& container, const T& value) {
  return std::find(container.begin(), container.end(), value) !=
         container.end();
}

}  // namespace midend::opt::util

#endif
