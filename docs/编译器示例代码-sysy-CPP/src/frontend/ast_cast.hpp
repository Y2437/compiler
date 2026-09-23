#ifndef BUAA_COMPILER_AST_CAST
#define BUAA_COMPILER_AST_CAST

#include <cassert>
#include <cstddef>

#include "frontend/ast.hpp"

namespace frontend::ast::cast {

template <typename T>
struct NodeTypeOf;

#define X(enum_name, class_name)                           \
  template <>                                              \
  struct NodeTypeOf<class_name> {                          \
    static constexpr NodeType value = NodeType::enum_name; \
  };
NODE_TABLE
#undef X

template <typename T>
inline bool isa(const Node& node) {
  return node.get_type() == NodeTypeOf<T>::value;
}

template <typename T>
inline bool isa(const Node* node) {
  return node != nullptr && isa<T>(*node);
}

template <typename T>
inline auto cast(Node& node) -> T& {
  assert(isa<T>(node) && "invalid ast cast");
  return static_cast<T&>(node);
}

template <typename T>
inline auto cast(const Node& node) -> const T& {
  assert(isa<T>(node) && "invalid ast cast");
  return static_cast<const T&>(node);
}

template <typename T>
inline auto dyn_cast(Node* node) -> T* {
  return isa<T>(node) ? static_cast<T*>(node) : nullptr;
}

template <typename T>
inline auto dyn_cast(const Node* node) -> const T* {
  return isa<T>(node) ? static_cast<const T*>(node) : nullptr;
}

template <typename T>
inline auto child_as(Node& node, size_t index) -> T& {
  const auto& children = node.get_children();
  assert(index < children.size() && "child index out of range");
  assert(children[index] != nullptr && "child node is null");
  return cast<T>(*children[index]);
}

template <typename T>
inline auto child_as(const Node& node, size_t index) -> const T& {
  const auto& children = node.get_children();
  assert(index < children.size() && "child index out of range");
  assert(children[index] != nullptr && "child node is null");
  return cast<T>(*children[index]);
}

template <typename T>
inline auto child_dyn_as(Node& node, size_t index) -> T* {
  const auto& children = node.get_children();
  if (index >= children.size() || children[index] == nullptr) {
    return nullptr;
  }
  return dyn_cast<T>(children[index].get());
}

template <typename T>
inline auto child_dyn_as(const Node& node, size_t index) -> const T* {
  const auto& children = node.get_children();
  if (index >= children.size() || children[index] == nullptr) {
    return nullptr;
  }
  return dyn_cast<T>(children[index].get());
}

}  // namespace frontend::ast::cast

#endif