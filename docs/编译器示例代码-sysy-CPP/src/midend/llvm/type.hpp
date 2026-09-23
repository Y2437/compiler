#ifndef BUAA_COMPILER_LLVM_TYPE
#define BUAA_COMPILER_LLVM_TYPE

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace midend::llvm {

class Type {
 public:
  enum class TypeID {
    VOID_ID,
    LABEL_ID,
    INTEGER_ID,
    FUNCTION_ID,
    POINTER_ID,
    ARRAY_ID
  };

  virtual ~Type() = default;
  virtual auto to_string() const -> std::string = 0;
  virtual auto bits_num() const -> int = 0;

  auto get_type_id() const -> TypeID { return id; }
  auto is_void_ty() const -> bool { return id == TypeID::VOID_ID; }
  auto is_integer_ty() const -> bool { return id == TypeID::INTEGER_ID; }
  auto is_function_ty() const -> bool { return id == TypeID::FUNCTION_ID; }
  auto is_pointer_ty() const -> bool { return id == TypeID::POINTER_ID; }
  auto is_array_ty() const -> bool { return id == TypeID::ARRAY_ID; }

 protected:
  explicit Type(TypeID id) : id(id) {}

 private:
  TypeID id;
};

class VoidType : public Type {
 public:
  static auto get() -> std::shared_ptr<VoidType>;
  auto to_string() const -> std::string override;
  auto bits_num() const -> int override {
    throw std::runtime_error("VoidType has no bits_num");
  }

 private:
  VoidType() : Type(TypeID::VOID_ID) {}
};

class LabelType : public Type {
 public:
  static auto get() -> std::shared_ptr<LabelType>;
  auto to_string() const -> std::string override;
  auto bits_num() const -> int override {
    throw std::runtime_error("LabelType has no bits_num");
  }

 private:
  LabelType() : Type(TypeID::LABEL_ID) {}
};

class IntegerType : public Type {
 public:
  static auto get(int bits) -> std::shared_ptr<IntegerType>;
  auto to_string() const -> std::string override;
  auto bits_num() const -> int override { return bits; }

 private:
  explicit IntegerType(int bits) : Type(TypeID::INTEGER_ID), bits(bits) {}

 private:
  int bits;
};

class FunctionType : public Type {
 public:
  static auto get(const std::shared_ptr<Type>& return_type,
                  const std::vector<std::shared_ptr<Type>>& param_types)
      -> std::shared_ptr<FunctionType>;

  auto get_return_type() const -> std::shared_ptr<Type> { return return_type; }
  auto get_param_types() const -> const std::vector<std::shared_ptr<Type>>& {
    return param_types;
  }

  auto to_string() const -> std::string override;
  auto bits_num() const -> int override {
    throw std::runtime_error("FunctionType has no bits_num");
  }

 private:
  FunctionType(std::shared_ptr<Type> return_type,
               std::vector<std::shared_ptr<Type>> param_types)
      : Type(TypeID::FUNCTION_ID),
        return_type(std::move(return_type)),
        param_types(std::move(param_types)) {}

 private:
  std::shared_ptr<Type> return_type;
  std::vector<std::shared_ptr<Type>> param_types;
};

class PointerType : public Type {
 public:
  static auto get(const std::shared_ptr<Type>& reference_type)
      -> std::shared_ptr<PointerType>;

  auto get_reference_type() const -> std::shared_ptr<Type> {
    return reference_type;
  }
  auto to_string() const -> std::string override;
  auto bits_num() const -> int override { return 32; }

 private:
  explicit PointerType(std::shared_ptr<Type> reference_type)
      : Type(TypeID::POINTER_ID), reference_type(std::move(reference_type)) {}

 private:
  std::shared_ptr<Type> reference_type;
};

class ArrayType : public Type {
 public:
  static auto get(const std::shared_ptr<Type>& element_type, int size)
      -> std::shared_ptr<ArrayType>;

  auto get_element_type() const -> std::shared_ptr<Type> {
    return element_type;
  }
  auto get_size() const -> int { return size; }

  auto to_string() const -> std::string override;
  auto bits_num() const -> int override {
    return size * element_type->bits_num();
  }

 private:
  ArrayType(std::shared_ptr<Type> element_type, int size)
      : Type(TypeID::ARRAY_ID),
        element_type(std::move(element_type)),
        size(size) {}

 private:
  std::shared_ptr<Type> element_type;
  int size;
};

}  // namespace midend::llvm

#endif  // BUAA_COMPILER_LLVM_TYPE
