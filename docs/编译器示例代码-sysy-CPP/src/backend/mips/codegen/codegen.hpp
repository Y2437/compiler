#ifndef BUAA_COMPILER_MIPS_CODEGEN
#define BUAA_COMPILER_MIPS_CODEGEN

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "backend/mips/ir/function.hpp"
#include "backend/mips/ir/instruction.hpp"
#include "backend/mips/ir/module.hpp"
#include "backend/mips/ir/operand.hpp"
#include "midend/llvm/instruction.hpp"
#include "midend/llvm/module.hpp"
#include "midend/llvm/value.hpp"

namespace backend::mips {
namespace llvm = midend::llvm;

class CodeGenerator {
 public:
  auto generate(const llvm::Module& llvm_module) -> MipsModule;

 private:
  void clear_function_state() {
    value_map_.clear();
    stack_offset_map_.clear();
  }

  void gen_function(const std::shared_ptr<llvm::Function>& func);
  void gen_basic_block(const std::shared_ptr<llvm::BasicBlock>& bb);
  void gen_instruction(const std::shared_ptr<llvm::Instruction>& inst);

  void gen_ret(const std::shared_ptr<llvm::Ret>& ret);
  void gen_br(const std::shared_ptr<llvm::Br>& br);
  void gen_binary(const std::shared_ptr<llvm::Instruction>& inst);
  void gen_alloca(const std::shared_ptr<llvm::Alloca>& alloca_inst);
  void gen_load(const std::shared_ptr<llvm::Load>& load);
  void gen_store(const std::shared_ptr<llvm::Store>& store);
  void gen_icmp(const std::shared_ptr<llvm::ICmp>& icmp);
  void gen_call(const std::shared_ptr<llvm::Call>& call);
  void gen_getelementptr(const std::shared_ptr<llvm::Getelementptr>& gep);
  void gen_zext(const std::shared_ptr<llvm::Instruction>& zext);
  void gen_trunc(const std::shared_ptr<llvm::Instruction>& trunc);
  void gen_move(const std::shared_ptr<llvm::Move>& move);

  void gen_global_variable(const std::shared_ptr<llvm::GlobalVariable>& gv);

  void gen_constant_array_values(
      const std::shared_ptr<llvm::ConstantArray>& arr,
      std::vector<int>& values);

  auto get_operand(const std::shared_ptr<llvm::Value>& value)
      -> std::shared_ptr<RegOperand>;

  auto load_to_reg(const std::shared_ptr<llvm::Value>& value)
      -> std::shared_ptr<RegOperand>;

  auto alloc_virtual_reg(const std::shared_ptr<llvm::Value>& value)
      -> std::shared_ptr<RegOperand>;

  void emit(const MipsInstPtr& inst) { current_bb_->add_inst(inst); }
  void gen_prologue();
  void gen_epilogue();

  auto get_bb_label(const std::shared_ptr<llvm::BasicBlock>& bb)
      -> std::string {
    return "__cr_" + bb->get_name();
  }

  static auto clean_name(const std::string& name) -> std::string {
    if (!name.empty() && (name[0] == '@' || name[0] == '%')) {
      return name.substr(1);
    }
    return name;
  }

  MipsFunctionPtr current_func_;
  MipsBasicBlockPtr current_bb_;
  MipsModule mips_module_;
  std::unordered_map<int, std::shared_ptr<RegOperand>> value_map_;
  std::unordered_map<int, int> stack_offset_map_;
  std::unordered_map<int, std::string> global_var_map_;
  int string_counter_ = 0;
};

}  // namespace backend::mips

#endif
