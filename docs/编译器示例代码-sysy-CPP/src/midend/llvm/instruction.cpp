#include "midend/llvm/instruction.hpp"

#include <cassert>
#include <stdexcept>
#include <unordered_map>

namespace midend::llvm {  // creates

auto Ret::create(const std::shared_ptr<BasicBlock>& parent_block,
                 const std::shared_ptr<Value>& val, std::string name)
    -> std::shared_ptr<Ret> {
  auto ret = std::shared_ptr<Ret>(
      new Ret(parent_block, val->get_type(), std::move(name)));
  ret->add_operand(val);
  val->add_user(ret);
  return ret;
}

auto Ret::create(const std::shared_ptr<BasicBlock>& parent_block,
                 std::string name) -> std::shared_ptr<Ret> {
  return std::shared_ptr<Ret>(new Ret(parent_block, std::move(name)));
}

auto Br::create(const std::shared_ptr<BasicBlock>& parent_block,
                const std::shared_ptr<BasicBlock>& target, std::string name)
    -> std::shared_ptr<Br> {
  auto br = std::shared_ptr<Br>(new Br(parent_block, std::move(name)));
  br->add_operand(target);
  target->add_user(br);
  return br;
}

auto Br::create(const std::shared_ptr<BasicBlock>& parent_block,
                const std::shared_ptr<Value>& cond,
                const std::shared_ptr<BasicBlock>& true_target,
                const std::shared_ptr<BasicBlock>& false_target,
                std::string name) -> std::shared_ptr<Br> {
  auto br = std::shared_ptr<Br>(new Br(parent_block, std::move(name)));
  br->add_operand(cond);
  br->add_operand(true_target);
  br->add_operand(false_target);
  cond->add_user(br);
  true_target->add_user(br);
  false_target->add_user(br);
  return br;
}

auto Alloca::create(const std::shared_ptr<BasicBlock>& parent_block,
                    const std::shared_ptr<Type>& content_type, std::string name)
    -> std::shared_ptr<Alloca> {
  return std::shared_ptr<Alloca>(
      new Alloca(parent_block, content_type, std::move(name)));
}

auto Load::create(const std::shared_ptr<BasicBlock>& parent_block,
                  const std::shared_ptr<Value>& addr, std::string name)
    -> std::shared_ptr<Load> {
  auto loaded_type = std::static_pointer_cast<PointerType>(addr->get_type())
                         ->get_reference_type();
  auto load = std::shared_ptr<Load>(
      new Load(parent_block, loaded_type, std::move(name)));
  load->add_operand(addr);
  addr->add_user(load);
  return load;
}

auto Store::create(const std::shared_ptr<BasicBlock>& parent_block,
                   const std::shared_ptr<Value>& val,
                   const std::shared_ptr<Value>& addr, std::string name)
    -> std::shared_ptr<Store> {
  auto store = std::shared_ptr<Store>(new Store(parent_block, std::move(name)));
  store->add_operand(val);
  store->add_operand(addr);
  val->add_user(store);
  addr->add_user(store);
  return store;
}

auto ICmp::create(const std::shared_ptr<BasicBlock>& parent_block,
                  ICmpType cmp_type, const std::shared_ptr<Value>& lhs,
                  const std::shared_ptr<Value>& rhs, std::string name)
    -> std::shared_ptr<ICmp> {
  assert(lhs->get_type() == rhs->get_type());
  auto icmp =
      std::shared_ptr<ICmp>(new ICmp(parent_block, cmp_type, std::move(name)));
  icmp->add_operand(lhs);
  icmp->add_operand(rhs);
  lhs->add_user(icmp);
  rhs->add_user(icmp);
  return icmp;
}

auto Call::create(const std::shared_ptr<BasicBlock>& parent_block,
                  const std::shared_ptr<Function>& function,
                  const std::vector<std::shared_ptr<Value>>& args,
                  std::string name) -> std::shared_ptr<Call> {
  auto call = std::shared_ptr<Call>(
      new Call(parent_block,
               std::static_pointer_cast<FunctionType>(function->get_type())
                   ->get_return_type(),
               std::move(name)));

  call->add_operand(function);
  function->add_user(call);
  for (const auto& arg : args) {
    call->add_operand(arg);
    arg->add_user(call);
  }
  return call;
}

auto Getelementptr::create(const std::shared_ptr<BasicBlock>& parent_block,
                           const std::shared_ptr<Value>& ptr,
                           const std::vector<std::shared_ptr<Value>>& indexes,
                           std::string name) -> std::shared_ptr<Getelementptr> {
  auto gep_type = ptr->get_type();
  for (const auto& _ : indexes) {
    if (gep_type->is_pointer_ty()) {
      gep_type =
          std::static_pointer_cast<PointerType>(gep_type)->get_reference_type();
    } else if (gep_type->is_array_ty()) {
      gep_type =
          std::static_pointer_cast<ArrayType>(gep_type)->get_element_type();
    } else {
      throw std::runtime_error("invalid gep base type");
    }
  }

  auto gep = std::shared_ptr<Getelementptr>(new Getelementptr(
      parent_block, PointerType::get(gep_type), std::move(name)));
  gep->add_operand(ptr);
  ptr->add_user(gep);

  for (const auto& index : indexes) {
    gep->add_operand(index);
    index->add_user(gep);
  }
  return gep;
}

auto Phi::create(const std::shared_ptr<BasicBlock>& parent_block,
                 const std::shared_ptr<Type>& type, std::string name)
    -> std::shared_ptr<Phi> {
  return std::shared_ptr<Phi>(new Phi(parent_block, type, std::move(name)));
}

void Phi::add_incoming(const std::shared_ptr<Value>& value,
                       const std::shared_ptr<BasicBlock>& block) {
  add_operand(value);
  add_operand(block);
}

auto Phi::get_value_for_block(const std::shared_ptr<BasicBlock>& block) const
    -> std::shared_ptr<Value> {
  for (size_t i = 0; i < get_num_incoming(); ++i) {
    if (get_incoming_block(i) == block) {
      return get_incoming_value(i);
    }
  }
  return nullptr;
}

void Phi::remove_incoming_for_block(const std::shared_ptr<BasicBlock>& block) {
  for (size_t i = 0; i < get_num_incoming(); ++i) {
    if (get_incoming_block(i) == block) {
      operands.erase(operands.begin() + static_cast<std::ptrdiff_t>(i * 2),
                     operands.begin() + static_cast<std::ptrdiff_t>(i * 2 + 2));
      return;
    }
  }
}

auto PhiCopy::create(const std::shared_ptr<BasicBlock>& parent_block,
                     std::string name) -> std::shared_ptr<PhiCopy> {
  return std::shared_ptr<PhiCopy>(new PhiCopy(parent_block, std::move(name)));
}

void PhiCopy::add(const std::shared_ptr<Phi>& phi,
                  const std::shared_ptr<Value>& value) {
  phis.push_back(phi);
  values.push_back(value);
}

void PhiCopy::remove(const std::shared_ptr<Phi>& phi,
                     const std::shared_ptr<Value>& value) {
  for (size_t i = 0; i < phis.size(); ++i) {
    if (phis[i] == phi && values[i] == value) {
      phis.erase(phis.begin() + static_cast<std::ptrdiff_t>(i));
      values.erase(values.begin() + static_cast<std::ptrdiff_t>(i));
      return;
    }
  }
}

void PhiCopy::change_value(size_t index, const std::shared_ptr<Value>& value) {
  if (index < values.size()) {
    values[index] = value;
  }
}

auto Move::create(const std::shared_ptr<BasicBlock>& parent_block,
                  const std::shared_ptr<Value>& src,
                  const std::shared_ptr<Value>& dst, std::string name)
    -> std::shared_ptr<Move> {
  auto move = std::shared_ptr<Move>(
      new Move(parent_block, dst->get_type(), std::move(name)));
  move->add_operand(src);
  move->add_operand(dst);
  src->add_user(move);
  dst->add_user(move);
  return move;
}

}  // namespace midend::llvm

namespace midend::llvm {  // to_string implementations
auto Ret::to_string() const -> std::string {
  if (is_void) {
    return "ret void";
  }
  return "ret " + type->to_string() + " " + operands[0]->get_name();
}

auto Br::to_string() const -> std::string {
  if (!is_cond_branch()) {
    return "br label %" + operands[0]->get_name();
  }

  return "br i1 " + operands[0]->get_name() + ", label %" +
         operands[1]->get_name() + ", label %" + operands[2]->get_name();
}

auto Alloca::to_string() const -> std::string {
  return name + " = alloca " + get_content_type()->to_string();
}

auto Load::to_string() const -> std::string {
  auto pointer_type =
      std::static_pointer_cast<PointerType>(operands[0]->get_type());
  return name + " = load " + pointer_type->get_reference_type()->to_string() +
         ", " + pointer_type->to_string() + " " + operands[0]->get_name();
}

auto Store::to_string() const -> std::string {
  return "store " + operands[0]->get_type()->to_string() + " " +
         operands[0]->get_name() + ", " + operands[1]->get_type()->to_string() +
         " " + operands[1]->get_name();
}

auto ICmp::to_string() const -> std::string {
  static const std::unordered_map<ICmpType, std::string> kIcmpMap = {
      {ICmpType::EQ, "eq"},   {ICmpType::NE, "ne"},   {ICmpType::SGT, "sgt"},
      {ICmpType::SGE, "sge"}, {ICmpType::SLT, "slt"}, {ICmpType::SLE, "sle"},
      {ICmpType::UGT, "ugt"}, {ICmpType::UGE, "uge"}, {ICmpType::ULT, "ult"},
      {ICmpType::ULE, "ule"}};

  return name + " = icmp " + kIcmpMap.at(cmp_type) + " " +
         operands[0]->get_type()->to_string() + " " + operands[0]->get_name() +
         ", " + operands[1]->get_name();
}

auto Call::to_string() const -> std::string {
  std::string out;
  if (type != VoidType::get()) {
    out += name + " = ";
  }

  out += "call " + type->to_string() + " " + operands[0]->get_name() + "(";
  for (size_t i = 1; i < operands.size(); ++i) {
    out += operands[i]->get_type()->to_string() + " " + operands[i]->get_name();
    if (i + 1 != operands.size()) {
      out += ", ";
    }
  }
  out += ")";
  return out;
}

auto Getelementptr::to_string() const -> std::string {
  std::string out = name + " = getelementptr ";

  if (operands[0]->get_type()->is_pointer_ty()) {
    out += std::static_pointer_cast<PointerType>(operands[0]->get_type())
               ->get_reference_type()
               ->to_string();
  } else if (operands[0]->get_type()->is_array_ty()) {
    out += std::static_pointer_cast<ArrayType>(operands[0]->get_type())
               ->get_element_type()
               ->to_string();
  } else {
    assert(false && "invalid gep base type");
  }

  out += ", " + operands[0]->get_type()->to_string() + " " +
         operands[0]->get_name();
  for (size_t i = 1; i < operands.size(); ++i) {
    out += ", " + operands[i]->get_type()->to_string() + " " +
           operands[i]->get_name();
  }
  return out;
}

auto Phi::to_string() const -> std::string {
  std::string out = name + " = phi " + type->to_string() + " ";
  for (size_t i = 0; i < get_num_incoming(); ++i) {
    if (i > 0) {
      out += ", ";
    }
    out += "[ " + get_incoming_value(i)->get_name() + ", %" +
           get_incoming_block(i)->get_name() + " ]";
  }
  return out;
}

auto PhiCopy::to_string() const -> std::string {
  std::string out = "# phicopy: ";
  for (size_t i = 0; i < phis.size(); ++i) {
    if (i > 0) {
      out += ", ";
    }
    out += phis[i]->get_name() + " <- " + values[i]->get_name();
  }
  return out;
}

auto Move::to_string() const -> std::string {
  return "# move: " + operands[1]->get_name() + " <- " +
         operands[0]->get_name();
}

}  // namespace midend::llvm
