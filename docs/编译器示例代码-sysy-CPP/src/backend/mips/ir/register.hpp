#ifndef BUAA_COMPILER_MIPS_REGISTER
#define BUAA_COMPILER_MIPS_REGISTER

#include <string>
#include <vector>

namespace backend::mips {

// MIPS 物理寄存器枚举
enum class Reg {
  // $zero - 常数0
  ZERO = 0,
  // $at - 汇编器临时寄存器（保留）
  AT = 1,
  // $v0-$v1 - 返回值寄存器
  V0 = 2,
  V1 = 3,
  // $a0-$a3 - 参数寄存器
  A0 = 4,
  A1 = 5,
  A2 = 6,
  A3 = 7,
  // $t0-$t7 - 临时寄存器（caller-saved）
  T0 = 8,
  T1 = 9,
  T2 = 10,
  T3 = 11,
  T4 = 12,
  T5 = 13,
  T6 = 14,
  T7 = 15,
  // $s0-$s7 - 保存寄存器（callee-saved）
  S0 = 16,
  S1 = 17,
  S2 = 18,
  S3 = 19,
  S4 = 20,
  S5 = 21,
  S6 = 22,
  S7 = 23,
  // $t8-$t9 - 临时寄存器（caller-saved）
  T8 = 24,
  T9 = 25,
  // $k0-$k1 - 内核保留
  K0 = 26,
  K1 = 27,
  // $gp - 全局指针
  GP = 28,
  // $sp - 栈指针
  SP = 29,
  // $fp - 帧指针
  FP = 30,
  // $ra - 返回地址
  RA = 31,

  // 虚拟寄存器起始标记（用于寄存器分配前）
  VIRTUAL_BASE = 100
};

// 获取寄存器名称
inline std::string reg_name(Reg reg) {
  static const char* names[] = {
      "$zero", "$at", "$v0", "$v1",                              // 0-3
      "$a0",   "$a1", "$a2", "$a3",                              // 4-7
      "$t0",   "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",  // 8-15
      "$s0",   "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",  // 16-23
      "$t8",   "$t9", "$k0", "$k1",                              // 24-27
      "$gp",   "$sp", "$fp", "$ra"                               // 28-31
  };
  int idx = static_cast<int>(reg);
  if (idx >= 0 && idx <= 31) {
    return names[idx];
  }
  // 虚拟寄存器
  return "$v" + std::to_string(idx - static_cast<int>(Reg::VIRTUAL_BASE));
}

// 判断是否为临时寄存器（caller-saved）
inline bool is_temp_reg(Reg reg) {
  int idx = static_cast<int>(reg);
  return (idx >= 8 && idx <= 15) || (idx >= 24 && idx <= 25);
}

// 判断是否为保存寄存器（callee-saved）
inline bool is_saved_reg(Reg reg) {
  int idx = static_cast<int>(reg);
  return idx >= 16 && idx <= 23;
}

// 判断是否为参数寄存器
inline bool is_arg_reg(Reg reg) {
  int idx = static_cast<int>(reg);
  return idx >= 4 && idx <= 7;
}

// 判断是否为虚拟寄存器
inline bool is_virtual_reg(Reg reg) {
  return static_cast<int>(reg) >= static_cast<int>(Reg::VIRTUAL_BASE);
}

// 获取参数寄存器列表
inline std::vector<Reg> get_arg_regs() {
  return {Reg::A0, Reg::A1, Reg::A2, Reg::A3};
}

// 获取可分配的临时寄存器列表
inline std::vector<Reg> get_temp_regs() {
  return {Reg::T0, Reg::T1, Reg::T2, Reg::T3, Reg::T4,
          Reg::T5, Reg::T6, Reg::T7, Reg::T8, Reg::T9};
}

// 获取可分配的保存寄存器列表
inline std::vector<Reg> get_saved_regs() {
  return {Reg::S0, Reg::S1, Reg::S2, Reg::S3,
          Reg::S4, Reg::S5, Reg::S6, Reg::S7};
}

// 获取所有可分配的通用寄存器列表
inline std::vector<Reg> get_allocatable_regs() {
  return {Reg::T0, Reg::T1, Reg::T2, Reg::T3, Reg::T4, Reg::T5,
          Reg::T6, Reg::T7, Reg::S0, Reg::S1, Reg::S2, Reg::S3,
          Reg::S4, Reg::S5, Reg::S6, Reg::S7, Reg::T8, Reg::T9};
}

}  // namespace backend::mips

#endif
