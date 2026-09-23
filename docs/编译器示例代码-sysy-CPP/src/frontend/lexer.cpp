#include "lexer.hpp"

#include <cctype>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "error.hpp"
#include "token.hpp"

namespace frontend::lexer {
using tokenMatch::try_match;

bool Lexer::next() {
  if (!token_lists.empty() && token_lists.back().is_EOF()) return false;

  while (skip_whitespace() || skip_comment());
  if (cur_loc.empty()) {
    token_lists.emplace_back(TokenType::EOFTK, "", cur_line + 1);
    return false;
  }

  if (auto result = try_match(cur_loc); result.has_value()) {
    if (result.has_error()) {
      // the only error in lexer
      report_error(ErrorType::LEX_INVALID_TOKEN, cur_line);
    }

    const auto& [len, type] = result.value();
    auto token = Token(type, std::string(cur_loc.substr(0, len)), cur_line);
    cur_loc.remove_prefix(len);
    token_lists.emplace_back(std::move(token));
    return true;
  }

  throw std::runtime_error("unreachable code in Lexer::next");
}

auto Lexer::peek(std::ptrdiff_t offset) -> const Token& {
  const auto position = static_cast<std::ptrdiff_t>(cur_idx) + offset;
  ENSURE(position >= 0, "cannot look before the start of the token stream");
  const auto target = static_cast<size_t>(position);

  while (target >= token_lists.size()) {
    if (!next()) {
      // next() returned false, meaning EOF was reached and added.
      ENSURE(!token_lists.empty() && token_lists.back().is_EOF(),
             "Expected EOF token");
      return token_lists.back();
    }
  }

  return token_lists[target];
}

void Lexer::advance() {
  if (peek(0).is_EOF()) return;
  cur_idx++;
}

void Lexer::output_tokens(std::ostream& os) {
  while (next());  // ensure all tokens are generated

  for (const auto& token : token_lists) {
    os << token << '\n';
  }
}

void Lexer::branch() { branch_stack.push(cur_idx); }
void Lexer::commit() {
  ENSURE(!branch_stack.empty(), "commit called without a corresponding branch");
  branch_stack.pop();
}
void Lexer::rollback() {
  ENSURE(!branch_stack.empty(),
         "rollback called without a corresponding branch");
  cur_idx = branch_stack.top();
  branch_stack.pop();
}

// skip ' ' \t \n \r \v \f
bool Lexer::skip_whitespace() {
  bool skipped = false;
  while (!cur_loc.empty() && isspace(cur_loc[0])) {
    if (cur_loc[0] == '\n') cur_line++;

    cur_loc.remove_prefix(1);
    skipped = true;
  }

  return skipped;
}

bool Lexer::skip_comment() {
  if (cur_loc.size() < 2 || cur_loc[0] != '/') return false;

  bool skipped = false;
  if (cur_loc[1] == '/') {
    size_t len = 2;
    while (len < cur_loc.size() && cur_loc[len] != '\n') len++;

    if (len < cur_loc.size()) {
      len++;
      cur_line++;
    }
    cur_loc.remove_prefix(len);
    skipped = true;
  } else if (cur_loc[1] == '*') {
    size_t len = 2;

    // TODO: error when not ending with */
    while (len < cur_loc.size() - 1) {
      if (cur_loc[len] == '\n') cur_line++;

      if (cur_loc[len] == '*' && cur_loc[len + 1] == '/') {
        len += 2;
        break;
      }
      len++;
    }

    cur_loc.remove_prefix(len);
    skipped = true;
  }

  return skipped;
}

}  // namespace frontend::lexer
