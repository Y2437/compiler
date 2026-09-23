#ifndef BUAA_COMPILER_MIPS_INSTRUCTION
#define BUAA_COMPILER_MIPS_INSTRUCTION

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "backend/mips/ir/operand.hpp"

namespace backend::mips {

class MipsInst;
using MipsInstPtr = std::shared_ptr<MipsInst>;

// MIPS 指令类型
enum class MipsInstType {
  // R-type 算术/逻辑指令
  ADDU,  // addu rd, rs, rt
  SUBU,  // subu rd, rs, rt
  MUL,   // mul rd, rs, rt (伪指令)
  DIV,   // div rs, rt
  MULT,  // mult rs, rt
  AND,   // and rd, rs, rt
  OR,    // or rd, rs, rt
  XOR,   // xor rd, rs, rt
  NOR,   // nor rd, rs, rt
  SLLV,  // sllv rd, rt, rs
  SRLV,  // srlv rd, rt, rs
  SRAV,  // srav rd, rt, rs
  SLL,   // sll rd, rt, shamt
  SRL,   // srl rd, rt, shamt
  SRA,   // sra rd, rt, shamt
  SLT,   // slt rd, rs, rt
  SLTU,  // sltu rd, rs, rt
  MFHI,  // mfhi rd
  MFLO,  // mflo rd

  // I-type 立即数指令
  ADDIU,  // addiu rt, rs, imm
  ANDI,   // andi rt, rs, imm
  ORI,    // ori rt, rs, imm
  XORI,   // xori rt, rs, imm
  SLTI,   // slti rt, rs, imm
  SLTIU,  // sltiu rt, rs, imm
  LUI,    // lui rt, imm

  // 内存访问指令
  LW,  // lw rt, offset(rs)
  SW,  // sw rt, offset(rs)
  LB,  // lb rt, offset(rs)
  SB,  // sb rt, offset(rs)

  // 分支跳转指令
  BEQ,   // beq rs, rt, label
  BNE,   // bne rs, rt, label
  BGEZ,  // bgez rs, label
  BGTZ,  // bgtz rs, label
  BLEZ,  // blez rs, label
  BLTZ,  // bltz rs, label
  J,     // j label
  JAL,   // jal label
  JR,    // jr rs
  JALR,  // jalr rd, rs

  // 伪指令（汇编器展开）
  LA,    // la rd, label
  LI,    // li rd, imm
  MOVE,  // move rd, rs
  NEG,   // neg rd, rs
  NOT,   // not rd, rs
  SEQ,   // seq rd, rs, rt
  SNE,   // sne rd, rs, rt
  SGE,   // sge rd, rs, rt
  SGT,   // sgt rd, rs, rt
  SLE,   // sle rd, rs, rt

  // 特殊指令
  SYSCALL,  // syscall
  NOP,      // nop
  LABEL,    // 标签（伪指令）
  COMMENT,  // 注释（用于调试）
};

// 获取指令助记符
inline std::string inst_name(MipsInstType type) {
  switch (type) {
    case MipsInstType::ADDU:
      return "addu";
    case MipsInstType::SUBU:
      return "subu";
    case MipsInstType::MUL:
      return "mul";
    case MipsInstType::DIV:
      return "div";
    case MipsInstType::MULT:
      return "mult";
    case MipsInstType::AND:
      return "and";
    case MipsInstType::OR:
      return "or";
    case MipsInstType::XOR:
      return "xor";
    case MipsInstType::NOR:
      return "nor";
    case MipsInstType::SLLV:
      return "sllv";
    case MipsInstType::SRLV:
      return "srlv";
    case MipsInstType::SRAV:
      return "srav";
    case MipsInstType::SLL:
      return "sll";
    case MipsInstType::SRL:
      return "srl";
    case MipsInstType::SRA:
      return "sra";
    case MipsInstType::SLT:
      return "slt";
    case MipsInstType::SLTU:
      return "sltu";
    case MipsInstType::MFHI:
      return "mfhi";
    case MipsInstType::MFLO:
      return "mflo";
    case MipsInstType::ADDIU:
      return "addiu";
    case MipsInstType::ANDI:
      return "andi";
    case MipsInstType::ORI:
      return "ori";
    case MipsInstType::XORI:
      return "xori";
    case MipsInstType::SLTI:
      return "slti";
    case MipsInstType::SLTIU:
      return "sltiu";
    case MipsInstType::LUI:
      return "lui";
    case MipsInstType::LW:
      return "lw";
    case MipsInstType::SW:
      return "sw";
    case MipsInstType::LB:
      return "lb";
    case MipsInstType::SB:
      return "sb";
    case MipsInstType::BEQ:
      return "beq";
    case MipsInstType::BNE:
      return "bne";
    case MipsInstType::BGEZ:
      return "bgez";
    case MipsInstType::BGTZ:
      return "bgtz";
    case MipsInstType::BLEZ:
      return "blez";
    case MipsInstType::BLTZ:
      return "bltz";
    case MipsInstType::J:
      return "j";
    case MipsInstType::JAL:
      return "jal";
    case MipsInstType::JR:
      return "jr";
    case MipsInstType::JALR:
      return "jalr";
    case MipsInstType::LA:
      return "la";
    case MipsInstType::LI:
      return "li";
    case MipsInstType::MOVE:
      return "move";
    case MipsInstType::NEG:
      return "neg";
    case MipsInstType::NOT:
      return "not";
    case MipsInstType::SEQ:
      return "seq";
    case MipsInstType::SNE:
      return "sne";
    case MipsInstType::SGE:
      return "sge";
    case MipsInstType::SGT:
      return "sgt";
    case MipsInstType::SLE:
      return "sle";
    case MipsInstType::SYSCALL:
      return "syscall";
    case MipsInstType::NOP:
      return "nop";
    case MipsInstType::LABEL:
      return "";
    case MipsInstType::COMMENT:
      return "#";
    default:
      return "unknown";
  }
}

// MIPS 指令基类
class MipsInst {
 public:
  virtual ~MipsInst() = default;
  MipsInst(MipsInstType type) : type_(type) {}
  virtual std::string to_string() const = 0;
  virtual MipsInstType get_type() const = 0;

  // 获取定义的寄存器
  virtual std::vector<std::shared_ptr<RegOperand>> get_defs() const {
    return {};
  }
  // 获取使用的寄存器
  virtual std::vector<std::shared_ptr<RegOperand>> get_uses() const {
    return {};
  }

  // 替换虚拟寄存器为物理寄存器
  virtual void replace_reg(int vreg_id, Reg preg) {}

 protected:
  MipsInstType type_;
};

// R-type 三操作数指令: op rd, rs, rt
class RTypeInst : public MipsInst {
 public:
  RTypeInst(MipsInstType type, std::shared_ptr<RegOperand> rd,
            std::shared_ptr<RegOperand> rs, std::shared_ptr<RegOperand> rt)
      : MipsInst(type),
        rd_(std::move(rd)),
        rs_(std::move(rs)),
        rt_(std::move(rt)) {}

  std::string to_string() const override {
    return inst_name(type_) + " " + rd_->to_string() + ", " + rs_->to_string() +
           ", " + rt_->to_string();
  }

  MipsInstType get_type() const override { return type_; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    return {rd_};
  }
  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rs_, rt_};
  }

  std::shared_ptr<RegOperand> get_rd() const { return rd_; }
  std::shared_ptr<RegOperand> get_rs() const { return rs_; }
  std::shared_ptr<RegOperand> get_rt() const { return rt_; }

  void set_rd(std::shared_ptr<RegOperand> rd) { rd_ = std::move(rd); }
  void set_rs(std::shared_ptr<RegOperand> rs) { rs_ = std::move(rs); }
  void set_rt(std::shared_ptr<RegOperand> rt) { rt_ = std::move(rt); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rd_ && rd_->is_virtual() && rd_->get_virtual_id() == vreg_id)
      rd_ = RegOperand::create(preg);
    if (rs_ && rs_->is_virtual() && rs_->get_virtual_id() == vreg_id)
      rs_ = RegOperand::create(preg);
    if (rt_ && rt_->is_virtual() && rt_->get_virtual_id() == vreg_id)
      rt_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rd,
                            std::shared_ptr<RegOperand> rs,
                            std::shared_ptr<RegOperand> rt) {
    return std::make_shared<RTypeInst>(type, std::move(rd), std::move(rs),
                                       std::move(rt));
  }

 private:
  std::shared_ptr<RegOperand> rd_;
  std::shared_ptr<RegOperand> rs_;
  std::shared_ptr<RegOperand> rt_;
};

// 移位指令: op rd, rt, shamt
class ShiftInst : public MipsInst {
 public:
  ShiftInst(MipsInstType type, std::shared_ptr<RegOperand> rd,
            std::shared_ptr<RegOperand> rt, int shamt)
      : MipsInst(type), rd_(std::move(rd)), rt_(std::move(rt)), shamt_(shamt) {}

  std::string to_string() const override {
    return inst_name(type_) + " " + rd_->to_string() + ", " + rt_->to_string() +
           ", " + std::to_string(shamt_);
  }

  MipsInstType get_type() const override { return type_; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    return {rd_};
  }
  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rt_};
  }

  std::shared_ptr<RegOperand> get_rd() const { return rd_; }
  std::shared_ptr<RegOperand> get_rt() const { return rt_; }

  void set_rd(std::shared_ptr<RegOperand> rd) { rd_ = std::move(rd); }
  void set_rt(std::shared_ptr<RegOperand> rt) { rt_ = std::move(rt); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rd_ && rd_->is_virtual() && rd_->get_virtual_id() == vreg_id)
      rd_ = RegOperand::create(preg);
    if (rt_ && rt_->is_virtual() && rt_->get_virtual_id() == vreg_id)
      rt_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rd,
                            std::shared_ptr<RegOperand> rt, int shamt) {
    return std::make_shared<ShiftInst>(type, std::move(rd), std::move(rt),
                                       shamt);
  }

 private:
  std::shared_ptr<RegOperand> rd_;
  std::shared_ptr<RegOperand> rt_;
  int shamt_;
};

// I-type 指令: op rt, rs, imm
class ITypeInst : public MipsInst {
 public:
  ITypeInst(MipsInstType type, std::shared_ptr<RegOperand> rt,
            std::shared_ptr<RegOperand> rs, std::shared_ptr<ImmOperand> imm)
      : MipsInst(type),
        rt_(std::move(rt)),
        rs_(std::move(rs)),
        imm_(std::move(imm)) {}

  std::string to_string() const override {
    return inst_name(type_) + " " + rt_->to_string() + ", " + rs_->to_string() +
           ", " + imm_->to_string();
  }

  MipsInstType get_type() const override { return type_; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    return {rt_};
  }
  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rs_};
  }

  std::shared_ptr<RegOperand> get_rt() const { return rt_; }
  std::shared_ptr<RegOperand> get_rs() const { return rs_; }
  std::shared_ptr<ImmOperand> get_imm() const { return imm_; }

  void set_rt(std::shared_ptr<RegOperand> rt) { rt_ = std::move(rt); }
  void set_rs(std::shared_ptr<RegOperand> rs) { rs_ = std::move(rs); }
  void set_imm(std::shared_ptr<ImmOperand> imm) { imm_ = std::move(imm); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rt_ && rt_->is_virtual() && rt_->get_virtual_id() == vreg_id)
      rt_ = RegOperand::create(preg);
    if (rs_ && rs_->is_virtual() && rs_->get_virtual_id() == vreg_id)
      rs_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rt,
                            std::shared_ptr<RegOperand> rs,
                            std::shared_ptr<ImmOperand> imm) {
    return std::make_shared<ITypeInst>(type, std::move(rt), std::move(rs),
                                       std::move(imm));
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rt,
                            std::shared_ptr<RegOperand> rs, int imm) {
    return std::make_shared<ITypeInst>(type, std::move(rt), std::move(rs),
                                       ImmOperand::create(imm));
  }

 private:
  std::shared_ptr<RegOperand> rt_;
  std::shared_ptr<RegOperand> rs_;
  std::shared_ptr<ImmOperand> imm_;
};

// LUI 指令: lui rt, imm
class LuiInst : public MipsInst {
 public:
  LuiInst(std::shared_ptr<RegOperand> rt, std::shared_ptr<ImmOperand> imm)
      : MipsInst(MipsInstType::LUI), rt_(std::move(rt)), imm_(std::move(imm)) {}

  std::string to_string() const override {
    return "lui " + rt_->to_string() + ", " + imm_->to_string();
  }

  MipsInstType get_type() const override { return MipsInstType::LUI; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    return {rt_};
  }

  std::shared_ptr<RegOperand> get_rt() const { return rt_; }
  void set_rt(std::shared_ptr<RegOperand> rt) { rt_ = std::move(rt); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rt_ && rt_->is_virtual() && rt_->get_virtual_id() == vreg_id)
      rt_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rt,
                            std::shared_ptr<ImmOperand> imm) {
    return std::make_shared<LuiInst>(std::move(rt), std::move(imm));
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rt, int imm) {
    return std::make_shared<LuiInst>(std::move(rt), ImmOperand::create(imm));
  }

 private:
  std::shared_ptr<RegOperand> rt_;
  std::shared_ptr<ImmOperand> imm_;
};

// 内存访问指令: lw/sw rt, offset(base)
class MemInst : public MipsInst {
 public:
  MemInst(MipsInstType type, std::shared_ptr<RegOperand> rt,
          std::shared_ptr<MemOperand> mem)
      : MipsInst(type), rt_(std::move(rt)), mem_(std::move(mem)) {}

  std::string to_string() const override {
    return inst_name(type_) + " " + rt_->to_string() + ", " + mem_->to_string();
  }

  MipsInstType get_type() const override { return type_; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    if (type_ == MipsInstType::LW || type_ == MipsInstType::LB) {
      return {rt_};
    }
    return {};
  }

  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    if (type_ == MipsInstType::SW || type_ == MipsInstType::SB) {
      return {rt_, mem_->get_base()};
    }
    return {mem_->get_base()};
  }

  std::shared_ptr<RegOperand> get_rt() const { return rt_; }
  std::shared_ptr<MemOperand> get_mem() const { return mem_; }

  void set_rt(std::shared_ptr<RegOperand> rt) { rt_ = std::move(rt); }
  void set_mem(std::shared_ptr<MemOperand> mem) { mem_ = std::move(mem); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rt_ && rt_->is_virtual() && rt_->get_virtual_id() == vreg_id)
      rt_ = RegOperand::create(preg);
    auto base = mem_->get_base();
    if (base && base->is_virtual() && base->get_virtual_id() == vreg_id)
      mem_->set_base(RegOperand::create(preg));
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rt,
                            std::shared_ptr<MemOperand> mem) {
    return std::make_shared<MemInst>(type, std::move(rt), std::move(mem));
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rt,
                            std::shared_ptr<RegOperand> base, int offset) {
    return std::make_shared<MemInst>(
        type, std::move(rt), MemOperand::create(std::move(base), offset));
  }

 private:
  std::shared_ptr<RegOperand> rt_;
  std::shared_ptr<MemOperand> mem_;
};

// 分支指令: beq/bne rs, rt, label
class BranchInst : public MipsInst {
 public:
  BranchInst(MipsInstType type, std::shared_ptr<RegOperand> rs,
             std::shared_ptr<RegOperand> rt,
             std::shared_ptr<LabelOperand> label)
      : MipsInst(type),
        rs_(std::move(rs)),
        rt_(std::move(rt)),
        label_(std::move(label)) {}

  std::string to_string() const override {
    return inst_name(type_) + " " + rs_->to_string() + ", " + rt_->to_string() +
           ", " + label_->to_string();
  }

  MipsInstType get_type() const override { return type_; }

  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rs_, rt_};
  }

  std::shared_ptr<RegOperand> get_rs() const { return rs_; }
  std::shared_ptr<RegOperand> get_rt() const { return rt_; }
  std::shared_ptr<LabelOperand> get_label() const { return label_; }

  void set_rs(std::shared_ptr<RegOperand> rs) { rs_ = std::move(rs); }
  void set_rt(std::shared_ptr<RegOperand> rt) { rt_ = std::move(rt); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rs_ && rs_->is_virtual() && rs_->get_virtual_id() == vreg_id)
      rs_ = RegOperand::create(preg);
    if (rt_ && rt_->is_virtual() && rt_->get_virtual_id() == vreg_id)
      rt_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rs,
                            std::shared_ptr<RegOperand> rt,
                            std::shared_ptr<LabelOperand> label) {
    return std::make_shared<BranchInst>(type, std::move(rs), std::move(rt),
                                        std::move(label));
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rs,
                            std::shared_ptr<RegOperand> rt,
                            const std::string& label) {
    return std::make_shared<BranchInst>(type, std::move(rs), std::move(rt),
                                        LabelOperand::create(label));
  }

 private:
  std::shared_ptr<RegOperand> rs_;
  std::shared_ptr<RegOperand> rt_;
  std::shared_ptr<LabelOperand> label_;
};

// 单操作数分支指令: bgez/bgtz/blez/bltz rs, label
class BranchZeroInst : public MipsInst {
 public:
  BranchZeroInst(MipsInstType type, std::shared_ptr<RegOperand> rs,
                 std::shared_ptr<LabelOperand> label)
      : MipsInst(type), rs_(std::move(rs)), label_(std::move(label)) {}

  std::string to_string() const override {
    return inst_name(type_) + " " + rs_->to_string() + ", " +
           label_->to_string();
  }

  MipsInstType get_type() const override { return type_; }

  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rs_};
  }

  std::shared_ptr<RegOperand> get_rs() const { return rs_; }
  void set_rs(std::shared_ptr<RegOperand> rs) { rs_ = std::move(rs); }
  std::shared_ptr<LabelOperand> get_label() const { return label_; }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rs_ && rs_->is_virtual() && rs_->get_virtual_id() == vreg_id)
      rs_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rs,
                            std::shared_ptr<LabelOperand> label) {
    return std::make_shared<BranchZeroInst>(type, std::move(rs),
                                            std::move(label));
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rs,
                            const std::string& label) {
    return std::make_shared<BranchZeroInst>(type, std::move(rs),
                                            LabelOperand::create(label));
  }

 private:
  std::shared_ptr<RegOperand> rs_;
  std::shared_ptr<LabelOperand> label_;
};

// 跳转指令: j/jal label
class JumpInst : public MipsInst {
 public:
  JumpInst(MipsInstType type, std::shared_ptr<LabelOperand> label)
      : MipsInst(type), label_(std::move(label)) {}

  std::string to_string() const override {
    return inst_name(type_) + " " + label_->to_string();
  }

  MipsInstType get_type() const override { return type_; }

  std::shared_ptr<LabelOperand> get_label() const { return label_; }

  static MipsInstPtr create(MipsInstType type,
                            std::shared_ptr<LabelOperand> label) {
    return std::make_shared<JumpInst>(type, std::move(label));
  }

  static MipsInstPtr create(MipsInstType type, const std::string& label) {
    return std::make_shared<JumpInst>(type, LabelOperand::create(label));
  }

 private:
  std::shared_ptr<LabelOperand> label_;
};

// 寄存器跳转指令: jr rs
class JumpRegInst : public MipsInst {
 public:
  explicit JumpRegInst(std::shared_ptr<RegOperand> rs)
      : MipsInst(MipsInstType::JR), rs_(std::move(rs)) {}

  std::string to_string() const override { return "jr " + rs_->to_string(); }

  MipsInstType get_type() const override { return MipsInstType::JR; }

  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rs_};
  }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rs_ && rs_->is_virtual() && rs_->get_virtual_id() == vreg_id)
      rs_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rs) {
    return std::make_shared<JumpRegInst>(std::move(rs));
  }

 private:
  std::shared_ptr<RegOperand> rs_;
};

// 乘法指令 (HI/LO): mult rs, rt (结果在 HI/LO)
class MultInst : public MipsInst {
 public:
  MultInst(std::shared_ptr<RegOperand> rs, std::shared_ptr<RegOperand> rt)
      : MipsInst(MipsInstType::MULT), rs_(std::move(rs)), rt_(std::move(rt)) {}

  std::string to_string() const override {
    return "mult " + rs_->to_string() + ", " + rt_->to_string();
  }

  MipsInstType get_type() const override { return MipsInstType::MULT; }

  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rs_, rt_};
  }

  std::shared_ptr<RegOperand> get_rs() const { return rs_; }
  std::shared_ptr<RegOperand> get_rt() const { return rt_; }

  void set_rs(std::shared_ptr<RegOperand> rs) { rs_ = std::move(rs); }
  void set_rt(std::shared_ptr<RegOperand> rt) { rt_ = std::move(rt); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rs_ && rs_->is_virtual() && rs_->get_virtual_id() == vreg_id)
      rs_ = RegOperand::create(preg);
    if (rt_ && rt_->is_virtual() && rt_->get_virtual_id() == vreg_id)
      rt_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rs,
                            std::shared_ptr<RegOperand> rt) {
    return std::make_shared<MultInst>(std::move(rs), std::move(rt));
  }

 private:
  std::shared_ptr<RegOperand> rs_;
  std::shared_ptr<RegOperand> rt_;
};

// 除法指令: div rs, rt (结果在 HI/LO)
class DivInst : public MipsInst {
 public:
  DivInst(std::shared_ptr<RegOperand> rs, std::shared_ptr<RegOperand> rt)
      : MipsInst(MipsInstType::DIV), rs_(std::move(rs)), rt_(std::move(rt)) {}

  std::string to_string() const override {
    return "div " + rs_->to_string() + ", " + rt_->to_string();
  }

  MipsInstType get_type() const override { return MipsInstType::DIV; }

  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rs_, rt_};
  }

  std::shared_ptr<RegOperand> get_rs() const { return rs_; }
  std::shared_ptr<RegOperand> get_rt() const { return rt_; }

  void set_rs(std::shared_ptr<RegOperand> rs) { rs_ = std::move(rs); }
  void set_rt(std::shared_ptr<RegOperand> rt) { rt_ = std::move(rt); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rs_ && rs_->is_virtual() && rs_->get_virtual_id() == vreg_id)
      rs_ = RegOperand::create(preg);
    if (rt_ && rt_->is_virtual() && rt_->get_virtual_id() == vreg_id)
      rt_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rs,
                            std::shared_ptr<RegOperand> rt) {
    return std::make_shared<DivInst>(std::move(rs), std::move(rt));
  }

 private:
  std::shared_ptr<RegOperand> rs_;
  std::shared_ptr<RegOperand> rt_;
};

// MFHI/MFLO 指令: mfhi/mflo rd
class MfInst : public MipsInst {
 public:
  MfInst(MipsInstType type, std::shared_ptr<RegOperand> rd)
      : MipsInst(type), rd_(std::move(rd)) {}

  std::string to_string() const override {
    return inst_name(type_) + " " + rd_->to_string();
  }

  MipsInstType get_type() const override { return type_; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    return {rd_};
  }

  std::shared_ptr<RegOperand> get_rd() const { return rd_; }
  void set_rd(std::shared_ptr<RegOperand> rd) { rd_ = std::move(rd); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rd_ && rd_->is_virtual() && rd_->get_virtual_id() == vreg_id)
      rd_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(MipsInstType type, std::shared_ptr<RegOperand> rd) {
    return std::make_shared<MfInst>(type, std::move(rd));
  }

 private:
  std::shared_ptr<RegOperand> rd_;
};

// LA 伪指令: la rd, label
class LaInst : public MipsInst {
 public:
  LaInst(std::shared_ptr<RegOperand> rd, std::shared_ptr<LabelOperand> label)
      : MipsInst(MipsInstType::LA),
        rd_(std::move(rd)),
        label_(std::move(label)) {}

  std::string to_string() const override {
    return "la " + rd_->to_string() + ", " + label_->to_string();
  }

  MipsInstType get_type() const override { return MipsInstType::LA; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    return {rd_};
  }

  std::shared_ptr<RegOperand> get_rd() const { return rd_; }
  void set_rd(std::shared_ptr<RegOperand> rd) { rd_ = std::move(rd); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rd_ && rd_->is_virtual() && rd_->get_virtual_id() == vreg_id)
      rd_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rd,
                            std::shared_ptr<LabelOperand> label) {
    return std::make_shared<LaInst>(std::move(rd), std::move(label));
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rd,
                            const std::string& label) {
    return std::make_shared<LaInst>(std::move(rd), LabelOperand::create(label));
  }

 private:
  std::shared_ptr<RegOperand> rd_;
  std::shared_ptr<LabelOperand> label_;
};

// LI 伪指令: li rd, imm
class LiInst : public MipsInst {
 public:
  LiInst(std::shared_ptr<RegOperand> rd, std::shared_ptr<ImmOperand> imm)
      : MipsInst(MipsInstType::LI), rd_(std::move(rd)), imm_(std::move(imm)) {}

  std::string to_string() const override {
    return "li " + rd_->to_string() + ", " + imm_->to_string();
  }

  MipsInstType get_type() const override { return MipsInstType::LI; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    return {rd_};
  }

  std::shared_ptr<RegOperand> get_rd() const { return rd_; }
  std::shared_ptr<ImmOperand> get_imm() const { return imm_; }

  void set_rd(std::shared_ptr<RegOperand> rd) { rd_ = std::move(rd); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rd_ && rd_->is_virtual() && rd_->get_virtual_id() == vreg_id)
      rd_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rd,
                            std::shared_ptr<ImmOperand> imm) {
    return std::make_shared<LiInst>(std::move(rd), std::move(imm));
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rd, int imm) {
    return std::make_shared<LiInst>(std::move(rd), ImmOperand::create(imm));
  }

 private:
  std::shared_ptr<RegOperand> rd_;
  std::shared_ptr<ImmOperand> imm_;
};

// MOVE 伪指令: move rd, rs
class MoveInst : public MipsInst {
 public:
  MoveInst(std::shared_ptr<RegOperand> rd, std::shared_ptr<RegOperand> rs)
      : MipsInst(MipsInstType::MOVE), rd_(std::move(rd)), rs_(std::move(rs)) {}

  std::string to_string() const override {
    return "move " + rd_->to_string() + ", " + rs_->to_string();
  }

  MipsInstType get_type() const override { return MipsInstType::MOVE; }

  std::vector<std::shared_ptr<RegOperand>> get_defs() const override {
    return {rd_};
  }
  std::vector<std::shared_ptr<RegOperand>> get_uses() const override {
    return {rs_};
  }

  std::shared_ptr<RegOperand> get_rd() const { return rd_; }
  std::shared_ptr<RegOperand> get_rs() const { return rs_; }

  void set_rd(std::shared_ptr<RegOperand> rd) { rd_ = std::move(rd); }
  void set_rs(std::shared_ptr<RegOperand> rs) { rs_ = std::move(rs); }

  void replace_reg(int vreg_id, Reg preg) override {
    if (rd_ && rd_->is_virtual() && rd_->get_virtual_id() == vreg_id)
      rd_ = RegOperand::create(preg);
    if (rs_ && rs_->is_virtual() && rs_->get_virtual_id() == vreg_id)
      rs_ = RegOperand::create(preg);
  }

  static MipsInstPtr create(std::shared_ptr<RegOperand> rd,
                            std::shared_ptr<RegOperand> rs) {
    return std::make_shared<MoveInst>(std::move(rd), std::move(rs));
  }

 private:
  std::shared_ptr<RegOperand> rd_;
  std::shared_ptr<RegOperand> rs_;
};

// syscall 指令
class SyscallInst : public MipsInst {
 public:
  SyscallInst() : MipsInst(MipsInstType::SYSCALL) {}
  std::string to_string() const override { return "syscall"; }
  MipsInstType get_type() const override { return MipsInstType::SYSCALL; }

  static MipsInstPtr create() { return std::make_shared<SyscallInst>(); }
};

// nop 指令
class NopInst : public MipsInst {
 public:
  NopInst() : MipsInst(MipsInstType::NOP) {}
  std::string to_string() const override { return "nop"; }
  MipsInstType get_type() const override { return MipsInstType::NOP; }

  static MipsInstPtr create() { return std::make_shared<NopInst>(); }
};

// 标签（伪指令）
class LabelInst : public MipsInst {
 public:
  explicit LabelInst(std::string label)
      : MipsInst(MipsInstType::LABEL), label_(std::move(label)) {}

  std::string to_string() const override { return label_ + ":"; }
  MipsInstType get_type() const override { return MipsInstType::LABEL; }

  const std::string& get_label() const { return label_; }

  static MipsInstPtr create(const std::string& label) {
    return std::make_shared<LabelInst>(label);
  }

 private:
  std::string label_;
};

// 注释（用于调试）
class CommentInst : public MipsInst {
 public:
  explicit CommentInst(std::string comment)
      : MipsInst(MipsInstType::COMMENT), comment_(std::move(comment)) {}

  std::string to_string() const override { return "# " + comment_; }
  MipsInstType get_type() const override { return MipsInstType::COMMENT; }

  static MipsInstPtr create(const std::string& comment) {
    return std::make_shared<CommentInst>(comment);
  }

 private:
  std::string comment_;
};

}  // namespace backend::mips

#endif
