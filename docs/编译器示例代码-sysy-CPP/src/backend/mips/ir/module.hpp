#ifndef BUAA_COMPILER_MIPS_MODULE
#define BUAA_COMPILER_MIPS_MODULE

#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "backend/mips/ir/function.hpp"

namespace backend::mips {

struct WordData {
  std::vector<int> values;

  explicit WordData(int value) : values{value} {}
  explicit WordData(std::vector<int> vals) : values(std::move(vals)) {}
};

struct StringData {
  std::string value;

  explicit StringData(std::string str) : value(std::move(str)) {}
};

struct SpaceData {
  int size;

  explicit SpaceData(int sz) : size(sz) {}
};

class GlobalData {
 public:
  using DataVariant = std::variant<WordData, StringData, SpaceData>;

  static GlobalData word(const std::string& name, int value) {
    return GlobalData(name, WordData(value));
  }

  static GlobalData word_array(const std::string& name,
                               const std::vector<int>& values) {
    return GlobalData(name, WordData(values));
  }

  static GlobalData asciiz(const std::string& name, const std::string& value) {
    return GlobalData(name, StringData(value));
  }

  static GlobalData space(const std::string& name, int size) {
    return GlobalData(name, SpaceData(size));
  }

  const std::string& get_name() const { return name_; }
  const DataVariant& get_data() const { return data_; }

  std::string to_string() const {
    std::stringstream ss;
    ss << name_ << ": ";

    std::visit(
        [&ss](auto&& arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, WordData>) {
            ss << ".word ";
            for (size_t i = 0; i < arg.values.size(); ++i) {
              if (i > 0) ss << ", ";
              ss << arg.values[i];
            }
          } else if constexpr (std::is_same_v<T, StringData>) {
            ss << ".asciiz \"" << escape_string(arg.value) << "\"";
          } else if constexpr (std::is_same_v<T, SpaceData>) {
            ss << ".space " << arg.size;
          }
        },
        data_);

    return ss.str();
  }

 private:
  GlobalData(std::string name, DataVariant data)
      : name_(std::move(name)), data_(std::move(data)) {}

  static std::string escape_string(const std::string& str) {
    std::string result;
    for (char c : str) {
      switch (c) {
        case '\n':
          result += "\\n";
          break;
        case '\t':
          result += "\\t";
          break;
        case '\r':
          result += "\\r";
          break;
        case '\\':
          result += "\\\\";
          break;
        case '"':
          result += "\\\"";
          break;
        default:
          result += c;
          break;
      }
    }
    return result;
  }

  std::string name_;
  DataVariant data_;
};

class MipsModule {
 public:
  void add_global_data(const GlobalData& data) { global_data_.push_back(data); }
  void add_function(const MipsFunctionPtr& func) { functions_.push_back(func); }

  std::vector<MipsFunctionPtr>& get_functions() { return functions_; }
  const std::vector<MipsFunctionPtr>& get_functions() const {
    return functions_;
  }

  std::vector<GlobalData>& get_global_data() { return global_data_; }
  const std::vector<GlobalData>& get_global_data() const {
    return global_data_;
  }

  std::string to_string() const {
    std::stringstream ss;

    if (!global_data_.empty()) {
      ss << ".data\n";
      for (const auto& data : global_data_) {
        ss << data.to_string() << "\n";
      }
      ss << "\n";
    }

    ss << ".text\n";

    for (const auto& func : functions_) {
      if (func->get_name() == "main") {
        ss << func->to_string() << "\n";
        break;
      }
    }
    for (const auto& func : functions_) {
      if (func->get_name() != "main") {
        ss << func->to_string() << "\n";
      }
    }

    return ss.str();
  }

 private:
  std::vector<GlobalData> global_data_;
  std::vector<MipsFunctionPtr> functions_;
};

}  // namespace backend::mips

#endif
