#ifndef BUAA_COMPILER_TOKEN
#define BUAA_COMPILER_TOKEN

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace frontend::token {
// note: sort by length in descending order for longest match first
#define DELIMITER_TOKENS \
  X(AND, "&&")           \
  X(OR, "||")            \
  X(LEQ, "<=")           \
  X(GEQ, ">=")           \
  X(EQL, "==")           \
  X(NEQ, "!=")           \
                         \
  X(NOT, "!")            \
  X(PLUS, "+")           \
  X(MINU, "-")           \
  X(MULT, "*")           \
  X(DIV, "/")            \
  X(MOD, "%")            \
  X(LSS, "<")            \
  X(GRE, ">")            \
  X(SEMICN, ";")         \
  X(COMMA, ",")          \
  X(LPARENT, "(")        \
  X(RPARENT, ")")        \
  X(LBRACK, "[")         \
  X(RBRACK, "]")         \
  X(LBRACE, "{")         \
  X(RBRACE, "}")         \
  X(ASSIGN, "=")

#define RESERVED_TOKENS     \
  X(CONSTTK, "const")       \
  X(INTTK, "int")           \
  X(STATICTK, "static")     \
  X(BREAKTK, "break")       \
  X(CONTINUETK, "continue") \
  X(IFTK, "if")             \
  X(MAINTK, "main")         \
  X(ELSETK, "else")         \
  X(FORTK, "for")           \
  X(RETURNTK, "return")     \
  X(VOIDTK, "void")         \
  X(PRINTFTK, "printf")

#define OTHER_TOKENS  \
  X(IDENFR, Ident)    \
  X(INTCON, IntConst) \
  X(STRCON, StringConst)

}  // namespace frontend::token

namespace frontend::token {

enum class TokenType {
#define X(type, content) type,
  DELIMITER_TOKENS OTHER_TOKENS RESERVED_TOKENS
#undef X
      EOFTK,
};
std::ostream& operator<<(std::ostream& os, TokenType type);

using Keyword = std::pair<std::string_view, TokenType>;
constexpr Keyword RESERVED_WORDS[] = {
#define X(enum, content) {content, TokenType::enum},
    RESERVED_TOKENS
#undef X
};

// sort by length
constexpr Keyword DELIMITERS[] = {
#define X(enum, content) {content, TokenType::enum},
    DELIMITER_TOKENS
#undef X
};

class Token {
 public:
  Token() = default;
  Token(TokenType type, std::string content, size_t line)
      : type(type), content(std::move(content)), line_no(line) {}

  bool is_EOF() const { return type == TokenType::EOFTK; }
  auto get_type() const -> TokenType { return type; }
  auto get_content() const -> std::string { return content; }
  auto get_line_no() const -> size_t { return line_no; }

  friend std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token.type << ' ' << token.content;
    return os;
  }

  [[nodiscard]] bool operator==(TokenType type) const noexcept {
    return this->type == type;
  }
  [[nodiscard]] bool operator!=(TokenType type) const noexcept {
    return !(*this == type);
  }

 private:
  TokenType type;
  std::string content;
  size_t line_no;
};

}  // namespace frontend::token

namespace frontend::tokenMatch {
using token::TokenType;

class MatchResult {
 public:
  using valueType = std::pair<size_t, TokenType>;

  MatchResult(valueType value) : _value(value) {}
  MatchResult(valueType value, bool error) : _value(value), error(error) {}
  MatchResult(std::nullopt_t) : _value(std::nullopt) {}

  bool has_value() const { return _value.has_value(); }
  bool has_error() const { return error; }
  auto value() const -> valueType { return _value.value(); }

 private:
  std::optional<valueType> _value;  // name conflict with std::optional::value
  bool error = false;
};

auto try_match_reserved(std::string_view text) -> MatchResult;
auto try_match_identifier(std::string_view text) -> MatchResult;
auto try_match_str_const(std::string_view text) -> MatchResult;
auto try_match_int_const(std::string_view text) -> MatchResult;
auto try_match_delimiter(std::string_view text) -> MatchResult;

using MatchFunc = MatchResult (*)(std::string_view);
const MatchFunc MATCH_FUNCS[] = {
    try_match_reserved,  try_match_identifier, try_match_str_const,
    try_match_int_const, try_match_delimiter,
};

inline auto try_match(std::string_view text) -> MatchResult {
  for (const auto& match : MATCH_FUNCS) {
    auto res = match(text);
    if (res.has_value()) {
      return res;
    }
  }

  return std::nullopt;
}

}  // namespace frontend::tokenMatch

#endif
