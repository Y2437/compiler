#include "midend/llvm/type.hpp"

#include <unordered_map>

namespace midend::llvm {  // singletons

auto VoidType::get() -> std::shared_ptr<VoidType> {
  static std::shared_ptr<VoidType> instance(new VoidType());
  return instance;
}

auto LabelType::get() -> std::shared_ptr<LabelType> {
  static std::shared_ptr<LabelType> instance(new LabelType());
  return instance;
}

auto IntegerType::get(int bits) -> std::shared_ptr<IntegerType> {
  static std::unordered_map<int, std::shared_ptr<IntegerType>> cache;
  auto it = cache.find(bits);
  if (it != cache.end()) {
    return it->second;
  }

  auto created = std::shared_ptr<IntegerType>(new IntegerType(bits));
  cache.emplace(bits, created);
  return created;
}

auto FunctionType::get(const std::shared_ptr<Type>& return_type,
                       const std::vector<std::shared_ptr<Type>>& param_types)
    -> std::shared_ptr<FunctionType> {
  static std::unordered_map<std::string, std::shared_ptr<FunctionType>> cache;

  std::string key = return_type->to_string() + "(";
  for (const auto& type : param_types) {
    key += type->to_string() + ",";
  }
  key += ")";

  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }

  auto created = std::shared_ptr<FunctionType>(new FunctionType(
      return_type, std::vector<std::shared_ptr<Type>>(param_types)));
  cache.emplace(std::move(key), created);
  return created;
}

auto PointerType::get(const std::shared_ptr<Type>& reference_type)
    -> std::shared_ptr<PointerType> {
  static std::unordered_map<std::shared_ptr<Type>, std::shared_ptr<PointerType>>
      cache;
  auto it = cache.find(reference_type);
  if (it != cache.end()) {
    return it->second;
  }

  auto created = std::shared_ptr<PointerType>(new PointerType(reference_type));
  cache.emplace(reference_type, created);
  return created;
}

auto ArrayType::get(const std::shared_ptr<Type>& element_type, int size)
    -> std::shared_ptr<ArrayType> {
  static std::unordered_map<std::string, std::shared_ptr<ArrayType>> cache;
  std::string key =
      element_type->to_string() + "[" + std::to_string(size) + "]";

  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }

  auto created = std::shared_ptr<ArrayType>(new ArrayType(element_type, size));
  cache.emplace(std::move(key), created);
  return created;
}
}  // namespace midend::llvm

namespace midend::llvm {  // to_string
auto VoidType::to_string() const -> std::string { return "void"; }

auto LabelType::to_string() const -> std::string { return "label"; }

auto IntegerType::to_string() const -> std::string {
  return "i" + std::to_string(bits);
}

auto FunctionType::to_string() const -> std::string {
  return return_type->to_string();
}

auto PointerType::to_string() const -> std::string {
  return reference_type->to_string() + "*";
}

auto ArrayType::to_string() const -> std::string {
  return "[" + std::to_string(size) + " x " + element_type->to_string() + "]";
}

}  // namespace midend::llvm
