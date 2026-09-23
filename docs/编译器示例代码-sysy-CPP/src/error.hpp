#ifndef BUAA_COMPILER_ERROR
#define BUAA_COMPILER_ERROR

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#define ERROR_TABLE                                            \
  X(LEX_INVALID_TOKEN, a, "invalid token")                     \
  X(SEM_IDENT_REDEF, b, "ident redefined")                     \
  X(SEM_IDENT_UNDEF, c, "ident undefined")                     \
  X(SEM_FN_PARAM_NUM, d, "function parameter number mismatch") \
  X(SEM_FN_PARAM_TYPE, e, "function parameter type mismatch")  \
  X(SEM_FN_RET_TYPE, f, "function should not return value")    \
  X(SEM_FN_RET_MISS, g, "function missing return value")       \
  X(SEM_CONST_ASSIGN, h, "constant assignment")                \
  X(SYN_MISS_SIMICN, i, "missing semicolon")                   \
  X(SYN_MISS_PARENT, j, "missing parenthesis")                 \
  X(SYN_MISS_BRACK, k, "missing bracket")                      \
  X(SEM_PRINTF_MISMATCH, l, "printf format mismatch")          \
  X(SEM_BREAK_CONTINUE, m, "break or continue outside loop")

enum class ErrorType {
#define X(enum, code, desc) enum,
  ERROR_TABLE COUNT,
#undef X
};

constexpr std::pair<std::string_view, std::string_view> to_code_and_desc(
    ErrorType type) {
  constexpr std::pair<std::string_view, std::string_view> error_pairs[] = {
#define X(_enum, code, desc) {#code, desc},
      ERROR_TABLE
#undef X
  };
  auto index = static_cast<size_t>(type);
  return (index < static_cast<size_t>(ErrorType::COUNT))
             ? error_pairs[index]
             : std::pair<std::string_view, std::string_view>{"?", "?"};
}

class ErrorCollector {
 public:
  ErrorCollector() = default;

  void push(ErrorType type, size_t line_no) {
    if (enabled) errors.emplace_back(type, line_no);
  }
  bool has_error() const { return !errors.empty(); }

  // for peek ahead
  void disable() { enabled = false; }
  void enable() { enabled = true; }

  friend std::ostream& operator<<(std::ostream& os, ErrorCollector& ec) {
    // sort by line_no, then by error type
    std::sort(
        ec.errors.begin(), ec.errors.end(), [](const auto& a, const auto& b) {
          return a.second != b.second ? a.second < b.second : a.first < b.first;
        });

    for (const auto& [type, line_no] : ec.errors) {
      auto [code, desc] = to_code_and_desc(type);
      os << line_no << ' ' << code << '\n';
      // print to stderr as well
      // std::cerr << "Error at line " << line_no << ": " << desc << '\n';
    }

    return os;
  }

 private:
  std::vector<std::pair<ErrorType, size_t>> errors;
  bool enabled = true;
};

// global error collector in main.cpp
extern ErrorCollector ec;

inline void report_error(ErrorType type, size_t line_no) {
  ec.push(type, line_no);
}

#ifdef ENSURE
#undef ENSURE
#endif
#define ENSURE(cond, message)            \
  do {                                   \
    if (!(cond)) {                       \
      throw std::runtime_error(message); \
    }                                    \
  } while (0)

#endif
