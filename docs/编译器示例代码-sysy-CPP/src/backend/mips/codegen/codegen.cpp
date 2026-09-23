#include "backend/mips/codegen/codegen.hpp"

#include <cassert>
#include <memory>
#include <stdexcept>

#include "midend/llvm/type.hpp"

namespace backend::mips {

MipsModule CodeGenerator::generate(const llvm::Module& llvm_module) {
  mips_module_ = MipsModule();
  string_counter_ = 0;
  global_var_map_.clear();

  // 处理全局变量
  for (const auto& gv : llvm_module.get_global_variables()) {
    gen_global_variable(gv);
  }

  // 处理所有函数
  for (const auto& func : llvm_module.get_functions()) {
    gen_function(func);
  }

  return mips_module_;
}

void CodeGenerator::gen_function(const std::shared_ptr<llvm::Function>& func) {
  clear_function_state();

  // 创建 MIPS 函数
  std::string func_name = clean_name(func->get_name());
  current_func_ = MipsFunction::create(func_name);

  // 判断是否为叶函数（不调用其他函数）
  bool is_leaf = true;
  for (const auto& bb : func->get_basic_blocks()) {
    for (const auto& inst : bb->get_instructions()) {
      if (inst->get_instruction_type() ==
          llvm::Instruction::InstructionType::CALL) {
        is_leaf = false;
        // 统计调用参数数量以确定参数构造区大小
        auto call = std::dynamic_pointer_cast<llvm::Call>(inst);
        int arg_count =
            static_cast<int>(call->get_num_operands()) - 1;  // 减去函数本身
        current_func_->update_max_call_args(arg_count);
      }
    }
  }
  current_func_->get_stack_frame().need_save_ra = !is_leaf;

  // 设置参数数量
  current_func_->set_param_count(
      static_cast<int>(func->get_arguments().size()));

  // 第一遍：统计 alloca，分配栈空间
  for (const auto& bb : func->get_basic_blocks()) {
    for (const auto& inst : bb->get_instructions()) {
      if (auto alloca_inst = std::dynamic_pointer_cast<llvm::Alloca>(inst)) {
        auto content_type = std::dynamic_pointer_cast<llvm::PointerType>(
                                alloca_inst->get_type())
                                ->get_reference_type();
        int size = content_type->bits_num() / 8;
        int offset = current_func_->alloc_stack_slot(size);
        stack_offset_map_[alloca_inst->get_id()] = offset;
      }
    }
  }

  // 处理函数参数
  const auto& args = func->get_arguments();
  for (const auto& arg : args) {
    auto vreg = RegOperand::create_virtual(current_func_->new_virtual_reg());
    value_map_[arg->get_id()] = vreg;
  }

  // 计算栈帧大小
  current_func_->finalize_stack_frame();

  // 创建入口基本块并生成序言
  auto entry_bb = MipsBasicBlock::create(func_name);
  current_func_->add_basic_block(entry_bb);
  current_bb_ = entry_bb;

  // 生成函数序言
  gen_prologue();

  // 将参数从寄存器/栈复制到虚拟寄存器
  for (size_t i = 0; i < args.size(); ++i) {
    auto dst = value_map_[args[i]->get_id()];
    if (i < 4) {
      // 参数在 $a0-$a3
      Reg arg_reg = static_cast<Reg>(static_cast<int>(Reg::A0) + i);
      emit(MoveInst::create(dst, RegOperand::create(arg_reg)));
    } else {
      // 参数在栈上（调用者的栈帧中）
      int offset = current_func_->get_stack_frame().total_size +
                   (static_cast<int>(i) - 4) * 4;
      emit(MemInst::create(MipsInstType::LW, dst, RegOperand::create(Reg::SP),
                           offset));
    }
  }

  // 生成每个基本块
  bool first_bb = true;
  for (const auto& bb : func->get_basic_blocks()) {
    if (!first_bb) {
      // 创建新的 MIPS 基本块
      auto mips_bb = MipsBasicBlock::create(get_bb_label(bb));
      current_func_->add_basic_block(mips_bb);
      current_bb_ = mips_bb;
    }
    first_bb = false;

    gen_basic_block(bb);
  }

  mips_module_.add_function(current_func_);
}

void CodeGenerator::gen_basic_block(
    const std::shared_ptr<llvm::BasicBlock>& bb) {
  for (const auto& inst : bb->get_instructions()) {
    gen_instruction(inst);
  }
}

void CodeGenerator::gen_instruction(
    const std::shared_ptr<llvm::Instruction>& inst) {
  using InstType = llvm::Instruction::InstructionType;

  // 输出 LLVM IR 作为注释
  emit(CommentInst::create(inst->to_string()));

  switch (inst->get_instruction_type()) {
    case InstType::RET:
      gen_ret(std::dynamic_pointer_cast<llvm::Ret>(inst));
      break;
    case InstType::BR:
      gen_br(std::dynamic_pointer_cast<llvm::Br>(inst));
      break;
    case InstType::ADD:
    case InstType::SUB:
    case InstType::MUL:
    case InstType::SDIV:
    case InstType::SREM:
    case InstType::AND:
    case InstType::OR:
    case InstType::XOR:
    case InstType::SHL:
    case InstType::LSHR:
    case InstType::ASHR:
      gen_binary(inst);
      break;
    case InstType::ALLOCA:
      gen_alloca(std::dynamic_pointer_cast<llvm::Alloca>(inst));
      break;
    case InstType::LOAD:
      gen_load(std::dynamic_pointer_cast<llvm::Load>(inst));
      break;
    case InstType::STORE:
      gen_store(std::dynamic_pointer_cast<llvm::Store>(inst));
      break;
    case InstType::ICMP:
      gen_icmp(std::dynamic_pointer_cast<llvm::ICmp>(inst));
      break;
    case InstType::CALL:
      gen_call(std::dynamic_pointer_cast<llvm::Call>(inst));
      break;
    case InstType::GETELEMENTPTR:
      gen_getelementptr(std::dynamic_pointer_cast<llvm::Getelementptr>(inst));
      break;
    case InstType::ZEXT:
      gen_zext(inst);
      break;
    case InstType::TRUNC:
      gen_trunc(inst);
      break;
    case InstType::MOVE:
      gen_move(std::dynamic_pointer_cast<llvm::Move>(inst));
      break;
    default:
      throw std::runtime_error("unsupported IR instruction in MIPS backend");
  }
}

void CodeGenerator::gen_prologue() {
  const auto& frame = current_func_->get_stack_frame();
  if (frame.total_size > 0) {
    // addiu $sp, $sp, -frame_size
    emit(ITypeInst::create(MipsInstType::ADDIU, RegOperand::create(Reg::SP),
                           RegOperand::create(Reg::SP), -frame.total_size));
  }

  // 保存 $ra
  if (frame.need_save_ra) {
    emit(MemInst::create(MipsInstType::SW, RegOperand::create(Reg::RA),
                         RegOperand::create(Reg::SP), frame.ra_offset()));
  }

  // 保存 callee-saved 寄存器
  int offset = frame.saved_reg_offset();
  for (Reg reg : current_func_->get_saved_regs()) {
    emit(MemInst::create(MipsInstType::SW, RegOperand::create(reg),
                         RegOperand::create(Reg::SP), offset));
    offset += 4;
  }
}

void CodeGenerator::gen_epilogue() {
  const auto& frame = current_func_->get_stack_frame();

  // 恢复 callee-saved 寄存器
  int offset = frame.saved_reg_offset();
  for (Reg reg : current_func_->get_saved_regs()) {
    emit(MemInst::create(MipsInstType::LW, RegOperand::create(reg),
                         RegOperand::create(Reg::SP), offset));
    offset += 4;
  }

  // 恢复 $ra
  if (frame.need_save_ra) {
    emit(MemInst::create(MipsInstType::LW, RegOperand::create(Reg::RA),
                         RegOperand::create(Reg::SP), frame.ra_offset()));
  }

  // 恢复栈指针
  if (frame.total_size > 0) {
    emit(ITypeInst::create(MipsInstType::ADDIU, RegOperand::create(Reg::SP),
                           RegOperand::create(Reg::SP), frame.total_size));
  }

  // jr $ra
  emit(JumpRegInst::create(RegOperand::create(Reg::RA)));
}

void CodeGenerator::gen_ret(const std::shared_ptr<llvm::Ret>& ret) {
  // 如果有返回值，将其移动到 $v0
  if (ret->get_num_operands() > 0) {
    auto val = load_to_reg(ret->get_operand(0));
    emit(MoveInst::create(RegOperand::create(Reg::V0), val));
  }

  // 如果是 main 函数，生成退出 syscall
  if (current_func_->get_name() == "main") {
    // syscall 10: exit
    emit(LiInst::create(RegOperand::create(Reg::V0), 10));
    emit(SyscallInst::create());
  } else {
    // 生成函数尾声
    gen_epilogue();
  }
}

void CodeGenerator::gen_br(const std::shared_ptr<llvm::Br>& br) {
  if (!br->is_cond_branch()) {
    // 无条件跳转
    auto target =
        std::dynamic_pointer_cast<llvm::BasicBlock>(br->get_operand(0));
    emit(JumpInst::create(MipsInstType::J, get_bb_label(target)));
  } else {
    // 条件跳转
    auto cond = load_to_reg(br->get_operand(0));
    auto true_target =
        std::dynamic_pointer_cast<llvm::BasicBlock>(br->get_operand(1));
    auto false_target =
        std::dynamic_pointer_cast<llvm::BasicBlock>(br->get_operand(2));

    // bne cond, $zero, true_label
    emit(BranchInst::create(MipsInstType::BNE, cond,
                            RegOperand::create(Reg::ZERO),
                            get_bb_label(true_target)));
    // j false_label
    emit(JumpInst::create(MipsInstType::J, get_bb_label(false_target)));
  }
}

// 计算除以常数的魔数和移位量
// 使用算法参考: Hacker's Delight, Chapter 10
static std::tuple<int64_t, int, bool> compute_magic_number(int32_t divisor) {
  if (divisor == 0) return {0, 0, false};

  bool negative = divisor < 0;
  uint32_t d = negative ? static_cast<uint32_t>(-divisor)
                        : static_cast<uint32_t>(divisor);

  // 特殊处理 2 的幂次
  if ((d & (d - 1)) == 0) {
    int shift = 0;
    while ((1u << shift) < d) shift++;
    return {0, shift, negative};
  }

  // 计算 magic number
  uint32_t p = 31;
  uint64_t nc = (static_cast<uint64_t>(1) << 32) -
                ((static_cast<uint64_t>(1) << 32) % d) - 1;

  while (true) {
    p++;
    if (p >= 64) break;

    uint64_t two_p = static_cast<uint64_t>(1) << p;
    if (two_p > nc * (d - two_p % d)) {
      int64_t m = static_cast<int64_t>((two_p + d - two_p % d) / d);
      int shift = static_cast<int>(p - 32);
      return {m, shift, negative};
    }
  }

  return {0, 0, false};
}

void CodeGenerator::gen_binary(const std::shared_ptr<llvm::Instruction>& inst) {
  using InstType = llvm::Instruction::InstructionType;

  auto inst_type = inst->get_instruction_type();

  // 检查是否是除以常数的优化
  if (inst_type == InstType::SDIV || inst_type == InstType::SREM) {
    if (auto const_divisor = std::dynamic_pointer_cast<llvm::ConstantInt>(
            inst->get_operand(1))) {
      int32_t divisor = const_divisor->get_val();

      // 除数为 0 不优化
      if (divisor == 0) {
        goto fallback;
      }

      // 除数为 1 或 -1 的特殊处理
      if (divisor == 1) {
        auto lhs = load_to_reg(inst->get_operand(0));
        auto dst = alloc_virtual_reg(inst);
        if (inst_type == InstType::SDIV) {
          emit(MoveInst::create(dst, lhs));
        } else {
          emit(MoveInst::create(dst, RegOperand::create(Reg::ZERO)));
        }
        return;
      }
      if (divisor == -1) {
        auto lhs = load_to_reg(inst->get_operand(0));
        auto dst = alloc_virtual_reg(inst);
        if (inst_type == InstType::SDIV) {
          emit(RTypeInst::create(MipsInstType::SUBU, dst,
                                 RegOperand::create(Reg::ZERO), lhs));
        } else {
          emit(MoveInst::create(dst, RegOperand::create(Reg::ZERO)));
        }
        return;
      }

      bool is_negative = divisor < 0;
      uint32_t abs_divisor = is_negative ? static_cast<uint32_t>(-divisor)
                                         : static_cast<uint32_t>(divisor);

      // 检查是否是 2 的幂次
      if ((abs_divisor & (abs_divisor - 1)) == 0) {
        int shift = 0;
        while ((1u << shift) < abs_divisor) shift++;

        auto lhs = load_to_reg(inst->get_operand(0));
        auto dst = alloc_virtual_reg(inst);

        if (inst_type == InstType::SDIV) {
          // 有符号除法需要处理负数情况
          // q = (n + ((n >> 31) >>> (32 - shift))) >> shift
          auto sign =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(ShiftInst::create(MipsInstType::SRA, sign, lhs, 31));

          if (shift < 32) {
            auto correction =
                RegOperand::create_virtual(current_func_->new_virtual_reg());
            emit(ShiftInst::create(MipsInstType::SRL, correction, sign,
                                   32 - shift));

            auto adjusted =
                RegOperand::create_virtual(current_func_->new_virtual_reg());
            emit(RTypeInst::create(MipsInstType::ADDU, adjusted, lhs,
                                   correction));

            emit(ShiftInst::create(MipsInstType::SRA, dst, adjusted, shift));
          } else {
            emit(MoveInst::create(dst, lhs));
          }

          if (is_negative) {
            auto neg_result =
                RegOperand::create_virtual(current_func_->new_virtual_reg());
            emit(RTypeInst::create(MipsInstType::SUBU, neg_result,
                                   RegOperand::create(Reg::ZERO), dst));
            // 需要重新映射 dst
            value_map_[inst->get_id()] = neg_result;
          }
        } else {
          // 取模: r = n - (n / d) * d
          auto sign =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(ShiftInst::create(MipsInstType::SRA, sign, lhs, 31));

          auto correction =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(ShiftInst::create(MipsInstType::SRL, correction, sign,
                                 32 - shift));

          auto adjusted =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(
              RTypeInst::create(MipsInstType::ADDU, adjusted, lhs, correction));

          auto quotient =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(ShiftInst::create(MipsInstType::SRA, quotient, adjusted, shift));

          if (is_negative) {
            auto neg_q =
                RegOperand::create_virtual(current_func_->new_virtual_reg());
            emit(RTypeInst::create(MipsInstType::SUBU, neg_q,
                                   RegOperand::create(Reg::ZERO), quotient));
            quotient = neg_q;
          }

          // r = n - q * d
          auto qd =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          auto divisor_reg =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(LiInst::create(divisor_reg, divisor));
          emit(RTypeInst::create(MipsInstType::MUL, qd, quotient, divisor_reg));
          emit(RTypeInst::create(MipsInstType::SUBU, dst, lhs, qd));
        }
        return;
      }

      // 使用魔数优化
      auto [magic, shift, neg] = compute_magic_number(divisor);
      if (magic != 0) {
        auto lhs = load_to_reg(inst->get_operand(0));
        auto dst = alloc_virtual_reg(inst);

        // 加载魔数
        auto magic_reg =
            RegOperand::create_virtual(current_func_->new_virtual_reg());
        // 魔数可能超过 32 位，需要特殊处理
        int32_t magic32 = static_cast<int32_t>(magic);
        emit(LiInst::create(magic_reg, magic32));

        // mult lhs, magic_reg -> HI:LO
        emit(MultInst::create(lhs, magic_reg));

        // mfhi -> 获取高 32 位
        auto hi_val =
            RegOperand::create_virtual(current_func_->new_virtual_reg());
        emit(MfInst::create(MipsInstType::MFHI, hi_val));

        // 如果魔数溢出，需要加回 dividend
        std::shared_ptr<RegOperand> adjusted_hi = hi_val;
        if (magic > INT32_MAX) {
          adjusted_hi =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(RTypeInst::create(MipsInstType::ADDU, adjusted_hi, hi_val, lhs));
        }

        // 算术右移 shift 位
        auto shifted =
            RegOperand::create_virtual(current_func_->new_virtual_reg());
        if (shift > 0) {
          emit(ShiftInst::create(MipsInstType::SRA, shifted, adjusted_hi,
                                 shift));
        } else {
          shifted = adjusted_hi;
        }

        // 添加符号位校正: 如果结果为负，加 1
        auto sign_bit =
            RegOperand::create_virtual(current_func_->new_virtual_reg());
        emit(ShiftInst::create(MipsInstType::SRL, sign_bit, lhs, 31));

        auto quotient =
            RegOperand::create_virtual(current_func_->new_virtual_reg());
        emit(
            RTypeInst::create(MipsInstType::ADDU, quotient, shifted, sign_bit));

        if (inst_type == InstType::SDIV) {
          emit(MoveInst::create(dst, quotient));
        } else {
          // 取模: r = n - q * d
          auto divisor_reg =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(LiInst::create(divisor_reg, divisor));
          auto qd =
              RegOperand::create_virtual(current_func_->new_virtual_reg());
          emit(RTypeInst::create(MipsInstType::MUL, qd, quotient, divisor_reg));
          emit(RTypeInst::create(MipsInstType::SUBU, dst, lhs, qd));
        }
        return;
      }
    }
  }

fallback:
  auto lhs = load_to_reg(inst->get_operand(0));
  auto rhs = load_to_reg(inst->get_operand(1));
  auto dst = alloc_virtual_reg(inst);

  switch (inst_type) {
    case InstType::ADD:
      emit(RTypeInst::create(MipsInstType::ADDU, dst, lhs, rhs));
      break;
    case InstType::SUB:
      emit(RTypeInst::create(MipsInstType::SUBU, dst, lhs, rhs));
      break;
    case InstType::MUL:
      emit(RTypeInst::create(MipsInstType::MUL, dst, lhs, rhs));
      break;
    case InstType::SDIV:
      emit(DivInst::create(lhs, rhs));
      emit(MfInst::create(MipsInstType::MFLO, dst));
      break;
    case InstType::SREM:
      emit(DivInst::create(lhs, rhs));
      emit(MfInst::create(MipsInstType::MFHI, dst));
      break;
    case InstType::AND:
      emit(RTypeInst::create(MipsInstType::AND, dst, lhs, rhs));
      break;
    case InstType::OR:
      emit(RTypeInst::create(MipsInstType::OR, dst, lhs, rhs));
      break;
    case InstType::XOR:
      emit(RTypeInst::create(MipsInstType::XOR, dst, lhs, rhs));
      break;
    case InstType::SHL:
      emit(RTypeInst::create(MipsInstType::SLLV, dst, lhs, rhs));
      break;
    case InstType::LSHR:
      emit(RTypeInst::create(MipsInstType::SRLV, dst, lhs, rhs));
      break;
    case InstType::ASHR:
      emit(RTypeInst::create(MipsInstType::SRAV, dst, lhs, rhs));
      break;
    default:
      break;
  }
}

void CodeGenerator::gen_alloca(
    const std::shared_ptr<llvm::Alloca>& alloca_inst) {
  // alloca 在第一遍已经处理，这里只需要将栈地址存入虚拟寄存器
  auto dst = alloc_virtual_reg(alloca_inst);
  int offset = stack_offset_map_[alloca_inst->get_id()];

  // addiu dst, $sp, offset
  emit(ITypeInst::create(MipsInstType::ADDIU, dst, RegOperand::create(Reg::SP),
                         offset));
}

void CodeGenerator::gen_load(const std::shared_ptr<llvm::Load>& load) {
  auto dst = alloc_virtual_reg(load);
  auto addr = load_to_reg(load->get_operand(0));

  // lw dst, 0(addr)
  emit(MemInst::create(MipsInstType::LW, dst, addr, 0));
}

void CodeGenerator::gen_store(const std::shared_ptr<llvm::Store>& store) {
  auto val = load_to_reg(store->get_operand(0));
  auto addr = load_to_reg(store->get_operand(1));

  // sw val, 0(addr)
  emit(MemInst::create(MipsInstType::SW, val, addr, 0));
}

void CodeGenerator::gen_icmp(const std::shared_ptr<llvm::ICmp>& icmp) {
  auto dst = alloc_virtual_reg(icmp);
  auto lhs = load_to_reg(icmp->get_operand(0));
  auto rhs = load_to_reg(icmp->get_operand(1));

  using ICmpType = llvm::ICmp::ICmpType;

  switch (icmp->get_cmp_type()) {
    case ICmpType::EQ: {
      // xor tmp, lhs, rhs; sltiu dst, tmp, 1
      auto tmp = RegOperand::create_virtual(current_func_->new_virtual_reg());
      emit(RTypeInst::create(MipsInstType::XOR, tmp, lhs, rhs));
      emit(ITypeInst::create(MipsInstType::SLTIU, dst, tmp, 1));
      break;
    }
    case ICmpType::NE: {
      // xor tmp, lhs, rhs; sltu dst, $zero, tmp
      auto tmp = RegOperand::create_virtual(current_func_->new_virtual_reg());
      emit(RTypeInst::create(MipsInstType::XOR, tmp, lhs, rhs));
      emit(RTypeInst::create(MipsInstType::SLTU, dst,
                             RegOperand::create(Reg::ZERO), tmp));
      break;
    }
    case ICmpType::SLT:
      // slt dst, lhs, rhs
      emit(RTypeInst::create(MipsInstType::SLT, dst, lhs, rhs));
      break;
    case ICmpType::SGE: {
      // slt tmp, lhs, rhs; xori dst, tmp, 1
      auto tmp = RegOperand::create_virtual(current_func_->new_virtual_reg());
      emit(RTypeInst::create(MipsInstType::SLT, tmp, lhs, rhs));
      emit(ITypeInst::create(MipsInstType::XORI, dst, tmp, 1));
      break;
    }
    case ICmpType::SGT:
      // slt dst, rhs, lhs
      emit(RTypeInst::create(MipsInstType::SLT, dst, rhs, lhs));
      break;
    case ICmpType::SLE: {
      // slt tmp, rhs, lhs; xori dst, tmp, 1
      auto tmp = RegOperand::create_virtual(current_func_->new_virtual_reg());
      emit(RTypeInst::create(MipsInstType::SLT, tmp, rhs, lhs));
      emit(ITypeInst::create(MipsInstType::XORI, dst, tmp, 1));
      break;
    }
    case ICmpType::ULT:
      emit(RTypeInst::create(MipsInstType::SLTU, dst, lhs, rhs));
      break;
    case ICmpType::UGE: {
      auto tmp = RegOperand::create_virtual(current_func_->new_virtual_reg());
      emit(RTypeInst::create(MipsInstType::SLTU, tmp, lhs, rhs));
      emit(ITypeInst::create(MipsInstType::XORI, dst, tmp, 1));
      break;
    }
    case ICmpType::UGT:
      emit(RTypeInst::create(MipsInstType::SLTU, dst, rhs, lhs));
      break;
    case ICmpType::ULE: {
      auto tmp = RegOperand::create_virtual(current_func_->new_virtual_reg());
      emit(RTypeInst::create(MipsInstType::SLTU, tmp, rhs, lhs));
      emit(ITypeInst::create(MipsInstType::XORI, dst, tmp, 1));
      break;
    }
  }
}

void CodeGenerator::gen_call(const std::shared_ptr<llvm::Call>& call) {
  // 获取被调用的函数
  auto callee = std::dynamic_pointer_cast<llvm::Function>(call->get_operand(0));
  std::string func_name = clean_name(callee->get_name());

  // 检查是否是系统库函数，使用 MARS syscall
  if (func_name == "putint") {
    // syscall 1: print integer
    // $a0 = integer to print
    auto arg = load_to_reg(call->get_operand(1));
    emit(MoveInst::create(RegOperand::create(Reg::A0), arg));
    emit(LiInst::create(RegOperand::create(Reg::V0), 1));
    emit(SyscallInst::create());
    return;
  } else if (func_name == "putch") {
    // syscall 11: print character
    // $a0 = character to print
    auto arg = load_to_reg(call->get_operand(1));
    emit(MoveInst::create(RegOperand::create(Reg::A0), arg));
    emit(LiInst::create(RegOperand::create(Reg::V0), 11));
    emit(SyscallInst::create());
    return;
  } else if (func_name == "putstr") {
    // syscall 4: print string
    // $a0 = address of null-terminated string
    auto arg = load_to_reg(call->get_operand(1));
    emit(MoveInst::create(RegOperand::create(Reg::A0), arg));
    emit(LiInst::create(RegOperand::create(Reg::V0), 4));
    emit(SyscallInst::create());
    return;
  } else if (func_name == "getint") {
    // syscall 5: read integer
    // result in $v0
    emit(LiInst::create(RegOperand::create(Reg::V0), 5));
    emit(SyscallInst::create());
    // 将结果存入虚拟寄存器
    if (!call->get_type()->is_void_ty()) {
      auto dst = alloc_virtual_reg(call);
      emit(MoveInst::create(dst, RegOperand::create(Reg::V0)));
    }
    return;
  } else if (func_name == "getch") {
    // syscall 12: read character
    // result in $v0
    emit(LiInst::create(RegOperand::create(Reg::V0), 12));
    emit(SyscallInst::create());
    // 将结果存入虚拟寄存器
    if (!call->get_type()->is_void_ty()) {
      auto dst = alloc_virtual_reg(call);
      emit(MoveInst::create(dst, RegOperand::create(Reg::V0)));
    }
    return;
  }

  // 普通函数调用
  // 参数数量
  int arg_count = static_cast<int>(call->get_num_operands()) - 1;

  // 设置参数
  for (int i = 0; i < arg_count; ++i) {
    auto arg = load_to_reg(call->get_operand(i + 1));
    if (i < 4) {
      // 前4个参数放入 $a0-$a3
      Reg arg_reg = static_cast<Reg>(static_cast<int>(Reg::A0) + i);
      emit(MoveInst::create(RegOperand::create(arg_reg), arg));
    } else {
      // 后续参数压栈
      int offset = (i - 4) * 4;
      emit(MemInst::create(MipsInstType::SW, arg, RegOperand::create(Reg::SP),
                           offset));
    }
  }

  // 调用函数
  emit(JumpInst::create(MipsInstType::JAL, func_name));

  // 如果有返回值，从 $v0 获取
  if (!call->get_type()->is_void_ty()) {
    auto dst = alloc_virtual_reg(call);
    emit(MoveInst::create(dst, RegOperand::create(Reg::V0)));
  }
}

void CodeGenerator::gen_getelementptr(
    const std::shared_ptr<llvm::Getelementptr>& gep) {
  auto dst = alloc_virtual_reg(gep);
  auto base = load_to_reg(gep->get_operand(0));
  auto base_type = gep->get_operand(0)->get_type();

  // 累积偏移量
  auto current_addr = base;

  // 获取基础类型
  assert(base_type->is_pointer_ty() && "GEP base type is not pointer");
  std::shared_ptr<llvm::Type> current_type =
      std::dynamic_pointer_cast<llvm::PointerType>(base_type)
          ->get_reference_type();

  // 遍历所有索引
  for (size_t i = 1; i < gep->get_num_operands(); ++i) {
    auto index_value = gep->get_operand(i);
    int element_size = current_type->bits_num() / 8;

    // 检查是否是常量索引
    if (auto const_int =
            std::dynamic_pointer_cast<llvm::ConstantInt>(index_value)) {
      int idx = const_int->get_val();
      if (idx != 0) {
        int byte_offset = idx * element_size;
        auto new_addr =
            RegOperand::create_virtual(current_func_->new_virtual_reg());
        emit(ITypeInst::create(MipsInstType::ADDIU, new_addr, current_addr,
                               byte_offset));
        current_addr = new_addr;
      }
    } else {
      // 变量索引
      auto index_reg = load_to_reg(index_value);
      auto offset_reg =
          RegOperand::create_virtual(current_func_->new_virtual_reg());

      // offset = index * element_size
      if (element_size == 4) {
        // 左移2位
        emit(ShiftInst::create(MipsInstType::SLL, offset_reg, index_reg, 2));
      } else if (element_size == 1) {
        emit(MoveInst::create(offset_reg, index_reg));
      } else {
        // 通用乘法
        auto size_reg =
            RegOperand::create_virtual(current_func_->new_virtual_reg());
        emit(LiInst::create(size_reg, element_size));
        emit(RTypeInst::create(MipsInstType::MUL, offset_reg, index_reg,
                               size_reg));
      }

      auto new_addr =
          RegOperand::create_virtual(current_func_->new_virtual_reg());
      emit(RTypeInst::create(MipsInstType::ADDU, new_addr, current_addr,
                             offset_reg));
      current_addr = new_addr;
    }

    // 更新当前类型
    if (current_type->is_array_ty()) {
      current_type = std::dynamic_pointer_cast<llvm::ArrayType>(current_type)
                         ->get_element_type();
    }
  }

  // 最终地址
  emit(MoveInst::create(dst, current_addr));
}

void CodeGenerator::gen_zext(const std::shared_ptr<llvm::Instruction>& zext) {
  // i1 -> i32: 直接复制（高位已经是0）
  auto dst = alloc_virtual_reg(zext);
  auto src = load_to_reg(zext->get_operand(0));
  emit(MoveInst::create(dst, src));
}

void CodeGenerator::gen_trunc(const std::shared_ptr<llvm::Instruction>& trunc) {
  // i32 -> i1: andi dst, src, 1
  auto dst = alloc_virtual_reg(trunc);
  auto src = load_to_reg(trunc->get_operand(0));
  emit(ITypeInst::create(MipsInstType::ANDI, dst, src, 1));
}

void CodeGenerator::gen_move(const std::shared_ptr<llvm::Move>& move) {
  // MOVE 指令是显式的数据移动
  auto src_val = move->src();
  auto dst_val = move->dst();

  auto src = load_to_reg(src_val);

  // 目标可能是 PHI 指令的结果
  auto dst = get_operand(dst_val);
  if (!dst) {
    dst = alloc_virtual_reg(dst_val);
  }

  emit(MoveInst::create(dst, src));
}

void CodeGenerator::gen_global_variable(
    const std::shared_ptr<llvm::GlobalVariable>& gv) {
  std::string name = clean_name(gv->get_name());
  global_var_map_[gv->get_id()] = name;

  auto init_value = gv->get_init_value();
  auto ptr_type = std::dynamic_pointer_cast<llvm::PointerType>(gv->get_type());
  auto ref_type = ptr_type->get_reference_type();

  if (ref_type->is_integer_ty()) {
    // 整数变量
    if (auto const_int =
            std::dynamic_pointer_cast<llvm::ConstantInt>(init_value)) {
      mips_module_.add_global_data(
          GlobalData::word(name, const_int->get_val()));
    } else if (std::dynamic_pointer_cast<llvm::ZeroInitializer>(init_value)) {
      mips_module_.add_global_data(GlobalData::word(name, 0));
    } else {
      // 默认为0
      mips_module_.add_global_data(GlobalData::word(name, 0));
    }
  } else if (ref_type->is_array_ty()) {
    auto arr_type = std::dynamic_pointer_cast<llvm::ArrayType>(ref_type);

    if (auto const_str =
            std::dynamic_pointer_cast<llvm::ConstantString>(init_value)) {
      // 字符串常量
      mips_module_.add_global_data(
          GlobalData::asciiz(name, const_str->get_string_value()));
    } else if (auto const_arr =
                   std::dynamic_pointer_cast<llvm::ConstantArray>(init_value)) {
      // 数组常量 - 生成一组 .word 指令
      std::vector<int> values;
      gen_constant_array_values(const_arr, values);
      // LLVM permits a short initializer and treats omitted elements as
      // zero. Reserve the complete source-language array in MIPS data,
      // otherwise a later global may occupy the missing tail.
      values.resize(static_cast<size_t>(arr_type->get_size()), 0);
      mips_module_.add_global_data(GlobalData::word_array(name, values));
    } else if (std::dynamic_pointer_cast<llvm::ZeroInitializer>(init_value)) {
      // 零初始化数组
      int total_size = arr_type->bits_num() / 8;
      mips_module_.add_global_data(GlobalData::space(name, total_size));
    } else {
      // 默认分配空间但不初始化
      int total_size = arr_type->bits_num() / 8;
      mips_module_.add_global_data(GlobalData::space(name, total_size));
    }
  }
}

// 递归展开 ConstantArray 的值为一维 int 数组
void CodeGenerator::gen_constant_array_values(
    const std::shared_ptr<llvm::ConstantArray>& arr, std::vector<int>& values) {
  for (const auto& elem : arr->get_values()) {
    if (auto const_int = std::dynamic_pointer_cast<llvm::ConstantInt>(elem)) {
      values.push_back(const_int->get_val());
    } else if (auto nested_arr =
                   std::dynamic_pointer_cast<llvm::ConstantArray>(elem)) {
      gen_constant_array_values(nested_arr, values);
    } else if (std::dynamic_pointer_cast<llvm::ZeroInitializer>(elem)) {
      // ZeroInitializer 在数组中需要根据类型展开
      auto elem_type = elem->get_type();
      int num_words = elem_type->bits_num() / 32;
      for (int i = 0; i < num_words; ++i) {
        values.push_back(0);
      }
    }
  }
}

std::shared_ptr<RegOperand> CodeGenerator::get_operand(
    const std::shared_ptr<llvm::Value>& value) {
  // 查找已分配的寄存器
  auto it = value_map_.find(value->get_id());
  if (it != value_map_.end()) {
    return it->second;
  }

  // 如果是常量，直接返回 nullptr（需要特殊处理）
  return nullptr;
}

std::shared_ptr<RegOperand> CodeGenerator::load_to_reg(
    const std::shared_ptr<llvm::Value>& value) {
  // 如果已经在寄存器中
  auto existing = get_operand(value);
  if (existing) {
    return existing;
  }

  // 如果是常量整数
  if (auto const_int = std::dynamic_pointer_cast<llvm::ConstantInt>(value)) {
    auto dst = RegOperand::create_virtual(current_func_->new_virtual_reg());
    int val = const_int->get_val();
    if (val == 0) {
      emit(MoveInst::create(dst, RegOperand::create(Reg::ZERO)));
    } else {
      emit(LiInst::create(dst, val));
    }
    return dst;
  }

  // 如果是全局变量
  auto gv_it = global_var_map_.find(value->get_id());
  if (gv_it != global_var_map_.end()) {
    auto dst = RegOperand::create_virtual(current_func_->new_virtual_reg());
    emit(LaInst::create(dst, gv_it->second));
    return dst;
  }

  // 其他情况 - 创建一个虚拟寄存器并记录
  auto dst = RegOperand::create_virtual(current_func_->new_virtual_reg());
  value_map_[value->get_id()] = dst;
  return dst;
}

std::shared_ptr<RegOperand> CodeGenerator::alloc_virtual_reg(
    const std::shared_ptr<llvm::Value>& value) {
  // An IR value has one virtual register. Reusing the mapping is essential
  // when a lowering path asks for the destination more than once.
  if (auto existing = get_operand(value)) {
    return existing;
  }
  auto vreg = RegOperand::create_virtual(current_func_->new_virtual_reg());
  value_map_[value->get_id()] = vreg;
  return vreg;
}

}  // namespace backend::mips
