#ifndef BUAA_COMPILER_MIPS_OPERAND
#define BUAA_COMPILER_MIPS_OPERAND

#include <memory>
#include <string>
#include <utility>

#include "backend/mips/ir/register.hpp"

namespace backend::mips {

class MipsOperand;
using MipsOperandPtr = std::shared_ptr<MipsOperand>;

enum class OperandType {
  REG,    // 寄存器（物理或虚拟）
  IMM,    // 立即数
  MEM,    // 内存地址 offset(base)
  LABEL,  // 标签（用于跳转/全局变量）
};

class MipsOperand {
 public:
  virtual ~MipsOperand() = default;
  virtual std::string to_string() const = 0;
  virtual OperandType get_type() const = 0;
  virtual MipsOperandPtr clone() const = 0;
};

class RegOperand : public MipsOperand {
 public:
  explicit RegOperand(Reg reg) : reg_(reg), virtual_id_(-1) {}
  explicit RegOperand(int virtual_id)
      : reg_(Reg::VIRTUAL_BASE), virtual_id_(virtual_id) {}

  std::string to_string() const override {
    if (is_virtual()) {
      return "$vr" + std::to_string(virtual_id_);
    }
    return reg_name(reg_);
  }

  OperandType get_type() const override { return OperandType::REG; }

  MipsOperandPtr clone() const override {
    if (is_virtual()) {
      return std::make_shared<RegOperand>(virtual_id_);
    }
    return std::make_shared<RegOperand>(reg_);
  }

  Reg get_reg() const { return reg_; }
  int get_virtual_id() const { return virtual_id_; }
  bool is_virtual() const { return virtual_id_ >= 0; }

  void set_reg(Reg reg) {
    reg_ = reg;
    virtual_id_ = -1;
  }

  static std::shared_ptr<RegOperand> create(Reg reg) {
    return std::make_shared<RegOperand>(reg);
  }
  static std::shared_ptr<RegOperand> create_virtual(int id) {
    return std::make_shared<RegOperand>(id);
  }

 private:
  Reg reg_;
  int virtual_id_;  // >= 0 表示虚拟寄存器
};

class ImmOperand : public MipsOperand {
 public:
  explicit ImmOperand(int value) : value_(value) {}

  std::string to_string() const override { return std::to_string(value_); }

  OperandType get_type() const override { return OperandType::IMM; }

  MipsOperandPtr clone() const override {
    return std::make_shared<ImmOperand>(value_);
  }

  int get_value() const { return value_; }

  bool fits_in_16bits() const { return value_ >= -32768 && value_ <= 32767; }
  bool fits_in_u16bits() const { return value_ >= 0 && value_ <= 65535; }

  static std::shared_ptr<ImmOperand> create(int value) {
    return std::make_shared<ImmOperand>(value);
  }

 private:
  int value_;
};

class MemOperand : public MipsOperand {
 public:
  MemOperand(std::shared_ptr<RegOperand> base, int offset)
      : base_(std::move(base)), offset_(offset) {}

  std::string to_string() const override {
    return std::to_string(offset_) + "(" + base_->to_string() + ")";
  }

  OperandType get_type() const override { return OperandType::MEM; }

  MipsOperandPtr clone() const override {
    return std::make_shared<MemOperand>(
        std::static_pointer_cast<RegOperand>(base_->clone()), offset_);
  }

  std::shared_ptr<RegOperand> get_base() const { return base_; }
  int get_offset() const { return offset_; }

  void set_base(std::shared_ptr<RegOperand> base) { base_ = std::move(base); }
  void set_offset(int offset) { offset_ = offset; }

  static std::shared_ptr<MemOperand> create(std::shared_ptr<RegOperand> base,
                                            int offset) {
    return std::make_shared<MemOperand>(std::move(base), offset);
  }

  static std::shared_ptr<MemOperand> create(Reg base, int offset) {
    return std::make_shared<MemOperand>(RegOperand::create(base), offset);
  }

 private:
  std::shared_ptr<RegOperand> base_;
  int offset_;
};

class LabelOperand : public MipsOperand {
 public:
  explicit LabelOperand(std::string label) : label_(std::move(label)) {}

  std::string to_string() const override { return label_; }

  OperandType get_type() const override { return OperandType::LABEL; }

  MipsOperandPtr clone() const override {
    return std::make_shared<LabelOperand>(label_);
  }

  const std::string& get_label() const { return label_; }

  static std::shared_ptr<LabelOperand> create(const std::string& label) {
    return std::make_shared<LabelOperand>(label);
  }

 private:
  std::string label_;
};

inline bool is_reg(const MipsOperandPtr& op) {
  return op && op->get_type() == OperandType::REG;
}
inline bool is_imm(const MipsOperandPtr& op) {
  return op && op->get_type() == OperandType::IMM;
}
inline bool is_mem(const MipsOperandPtr& op) {
  return op && op->get_type() == OperandType::MEM;
}
inline bool is_label(const MipsOperandPtr& op) {
  return op && op->get_type() == OperandType::LABEL;
}

inline std::shared_ptr<RegOperand> as_reg(const MipsOperandPtr& op) {
  return std::dynamic_pointer_cast<RegOperand>(op);
}
inline std::shared_ptr<ImmOperand> as_imm(const MipsOperandPtr& op) {
  return std::dynamic_pointer_cast<ImmOperand>(op);
}
inline std::shared_ptr<MemOperand> as_mem(const MipsOperandPtr& op) {
  return std::dynamic_pointer_cast<MemOperand>(op);
}
inline std::shared_ptr<LabelOperand> as_label(const MipsOperandPtr& op) {
  return std::dynamic_pointer_cast<LabelOperand>(op);
}

}  // namespace backend::mips

#endif
