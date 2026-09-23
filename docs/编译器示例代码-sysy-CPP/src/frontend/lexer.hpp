#ifndef BUAA_COMPILER_LEXER
#define BUAA_COMPILER_LEXER

#include <cstddef>
#include <ostream>
#include <stack>
#include <string_view>
#include <vector>

#include "token.hpp"

namespace frontend::lexer {
using token::Token;
using token::TokenType;

class Lexer {
 public:
  explicit Lexer(std::string_view text)
      : text(text), cur_loc(text), cur_line(1), cur_idx(0) {}

  auto peek(std::ptrdiff_t offset) -> const Token&;
  void advance();
  void output_tokens(std::ostream& os);

  // we need backtracking due to LVal and Exp ambiguity
  void branch();
  void commit();
  void rollback();

 private:
  bool next();
  bool skip_whitespace();
  bool skip_comment();

 private:
  std::string_view text;
  std::string_view cur_loc;        // current location
  std::vector<Token> token_lists;  // lazy tokenization
  std::stack<size_t> branch_stack;
  size_t cur_line;  // current line number
  size_t cur_idx;   // current token index
};

}  // namespace frontend::lexer

#endif
