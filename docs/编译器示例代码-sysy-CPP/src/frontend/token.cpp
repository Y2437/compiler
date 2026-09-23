#include "frontend/token.hpp"

#include <cctype>
#include <cstddef>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>

#include "error.hpp"

namespace frontend::token {
std::ostream& operator<<(std::ostream& os, TokenType type) {
  static std::string_view token_type_names[] = {
#define X(token, name) #token,
      DELIMITER_TOKENS OTHER_TOKENS RESERVED_TOKENS
#undef X
  };

  auto index = static_cast<size_t>(type);
  if (index < static_cast<size_t>(TokenType::EOFTK)) {
    os << token_type_names[index];
  }
  return os;
}
}  // namespace frontend::token

namespace frontend::tokenMatch {
using token::RESERVED_WORDS, token::DELIMITERS;

auto try_match_reserved(std::string_view text) -> MatchResult {
  if (text.empty() || !isalpha(text[0])) return std::nullopt;

  for (const auto& [word, token] : RESERVED_WORDS) {
    auto word_len = word.size();
    if (word_len <= text.size() && text.substr(0, word_len) == word) {
      if (word_len == text.size() ||
          (word_len < text.size() && text[word_len] != '_' &&
           !isalnum(text[word_len]))) {
        return std::make_pair(word_len, token);
      }
    }
  }
  return std::nullopt;
}

auto try_match_identifier(std::string_view text) -> MatchResult {
  if (text.empty() || (!isalpha(text[0]) && text[0] != '_'))
    return std::nullopt;

  size_t len = 1;
  while (len < text.size() && (isalnum(text[len]) || text[len] == '_')) len++;

  return std::make_pair(len, TokenType::IDENFR);
}

auto try_match_str_const(std::string_view text) -> MatchResult {
  if (text.empty() || text[0] != '\"') return std::nullopt;

  size_t len = 1;
  while (len < text.size() && text[len] != '\"') len++;

  ENSURE(len < text.size() && text[len] == '\"',
         "Missing closing quote for string constant");

  return std::make_pair(len + 1, TokenType::STRCON);
}

auto try_match_int_const(std::string_view text) -> MatchResult {
  if (text.empty() || !isdigit(text[0])) return std::nullopt;

  // TODO: error when number not ending with white space. e.g. 123abc
  size_t len = 1;
  if (text[0] != '0') {
    while (len < text.size() && isdigit(text[len])) len++;
  }

  return std::make_pair(len, TokenType::INTCON);
}

auto try_match_delimiter(std::string_view text) -> MatchResult {
  if (text.empty() || isalnum(text[0])) return std::nullopt;

  for (const auto& [delimiter, token] : DELIMITERS) {
    auto len = delimiter.size();
    if (len <= text.size() && text.substr(0, len) == delimiter) {
      return std::make_pair(len, token);
    }
  }

  // error handling for & and |
  if (text[0] == '&') {
    return {std::make_pair(1, TokenType::AND), true};
  } else if (text[0] == '|') {
    return {std::make_pair(1, TokenType::OR), true};
  }

  return std::nullopt;
}

}  // namespace frontend::tokenMatch
