#ifndef BUAA_COMPILER_LLVM_MODULE
#define BUAA_COMPILER_LLVM_MODULE

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "midend/llvm/value.hpp"

namespace midend::llvm {

class Module {
 public:
  void add_global_variable(const std::shared_ptr<GlobalVariable>& global_var) {
    global_variables.push_back(global_var);
  }

  void remove_global_variable(
      const std::shared_ptr<GlobalVariable>& global_var) {
    global_variables.erase(std::remove(global_variables.begin(),
                                       global_variables.end(), global_var),
                           global_variables.end());
  }

  void add_function(const std::shared_ptr<Function>& func) {
    functions.push_back(func);
  }

  auto get_functions() const -> const std::vector<std::shared_ptr<Function>>& {
    return functions;
  }

  auto get_global_variables() const
      -> const std::vector<std::shared_ptr<GlobalVariable>>& {
    return global_variables;
  }

  auto to_string() const -> std::string {
    std::string out;

    for (const auto& gv : global_variables) {
      out += gv->to_string() + "\n";
    }

    for (const auto& func : Function::get_lib_funcs()) {
      out += "declare dso_local " + func->get_return_type()->to_string() + " " +
             func->get_name() + "(";
      const auto param_types = func->get_param_types();
      for (size_t i = 0; i < param_types.size(); ++i) {
        if (i != 0) {
          out += ", ";
        }
        out += param_types[i]->to_string();
      }
      out += ")\n";
    }

    out += "\n";

    for (const auto& func : functions) {
      out += func->to_string();
    }

    return out;
  }

 private:
  std::vector<std::shared_ptr<Function>> functions;
  std::vector<std::shared_ptr<GlobalVariable>> global_variables;
};

}  // namespace midend::llvm

#endif  // BUAA_COMPILER_LLVM_MODULE
