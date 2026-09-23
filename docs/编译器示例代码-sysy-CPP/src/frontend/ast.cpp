#include "ast.hpp"

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string_view>

#include "error.hpp"
#include "lexer.hpp"
#include "midend/visitor.hpp"
#include "token.hpp"

namespace frontend::ast {
using token::TokenType;

std::ostream& operator<<(std::ostream& os, NodeType type) {
  static std::string_view node_type_names[] = {
#define X(_, name) #name,
      NODE_TABLE
#undef X
  };
  auto index = static_cast<size_t>(type);
  os << (index < static_cast<size_t>(NodeType::COUNT) ? node_type_names[index]
                                                      : "UNKNOWN");
  return os;
}

namespace {
constexpr bool is_missing_token_recoverable(TokenType expected) {
  return expected == TokenType::SEMICN || expected == TokenType::RPARENT ||
         expected == TokenType::RBRACK;
}

Token make_missing_token(TokenType expected, int line) {
  switch (expected) {
    case TokenType::SEMICN:
      report_error(ErrorType::SYN_MISS_SIMICN, line);
      return Token(TokenType::SEMICN, ";", line);
    case TokenType::RPARENT:
      report_error(ErrorType::SYN_MISS_PARENT, line);
      return Token(TokenType::RPARENT, ")", line);
    case TokenType::RBRACK:
      report_error(ErrorType::SYN_MISS_BRACK, line);
      return Token(TokenType::RBRACK, "]", line);
    default:
      throw std::runtime_error("unrecoverable missing token");
  }
}

void expect_token(Node* node, Lexer& lexer, TokenType expected) {
  const auto current = lexer.peek(0);
  if (current != expected) {
    if (is_missing_token_recoverable(expected)) {
      const int line = lexer.peek(-1).get_line_no();
      node->add_child(
          std::make_unique<TokenNode>(make_missing_token(expected, line)));
    } else {
      std::cerr << "Fatal Error: Unexpected token " << current << " at line "
                << current.get_line_no() << ", expected " << expected << ".\n";
      exit(1);
    }
    return;
  }
  node->add_node(std::make_unique<TokenNode>(), lexer);
}

constexpr bool is_btype(TokenType type) { return type == TokenType::INTTK; }
constexpr bool is_decl(TokenType type) {
  return type == TokenType::CONSTTK || is_btype(type) ||
         type == TokenType::STATICTK;
}
constexpr bool is_unary_op(TokenType type) {
  return type == TokenType::PLUS || type == TokenType::MINU ||
         type == TokenType::NOT;
}
constexpr bool is_number(TokenType type) { return type == TokenType::INTCON; }
constexpr bool is_primary_exp(TokenType type) {
  return type == TokenType::LPARENT || type == TokenType::IDENFR ||
         is_number(type);
}
constexpr bool is_unary_exp(TokenType type) {
  return is_primary_exp(type) || is_unary_op(type);
}
}  // namespace

#define SIMPLE_PARSE(className) void className::parse(Lexer& lexer)
#define ADD_NODE(className) add_node(std::make_unique<className>(), lexer)
#define EXPECT_TOKEN(tokenType) expect_token(this, lexer, tokenType)

SIMPLE_PARSE(TokenNode) {
  token = lexer.peek(0);
  lexer.advance();
}

// CompUnit     ::= {Decl} {FuncDef} MainFuncDef
SIMPLE_PARSE(CompUnit) {
  bool has_main = false;
  while (!lexer.peek(0).is_EOF()) {
    if (lexer.peek(1) == TokenType::MAINTK) {
      ADD_NODE(MainFuncDef);
      has_main = true;
    } else if (lexer.peek(2) == TokenType::LPARENT) {
      ADD_NODE(FuncDef);
    } else {
      ADD_NODE(Decl);
    }
  }

  ENSURE(has_main, "Missing main function");
}

// Decl         ::= ConstDecl | VarDecl
SIMPLE_PARSE(Decl) {
  if (lexer.peek(0) == TokenType::CONSTTK) {
    ADD_NODE(ConstDecl);
  } else {
    ADD_NODE(VarDecl);
  }
}

// ConstDecl    ::= 'const' BType ConstDef {',' ConstDef} ';'
SIMPLE_PARSE(ConstDecl) {
  EXPECT_TOKEN(TokenType::CONSTTK);
  ADD_NODE(BType);
  ADD_NODE(ConstDef);

  while (lexer.peek(0) == TokenType::COMMA) {
    ADD_NODE(TokenNode);
    ADD_NODE(ConstDef);
  }

  EXPECT_TOKEN(TokenType::SEMICN);
}

// BType        ::= 'int'
SIMPLE_PARSE(BType) { EXPECT_TOKEN(TokenType::INTTK); }

// ConstDef     ::= Ident [ '[' ConstExp ']' ] '=' ConstInitVal
SIMPLE_PARSE(ConstDef) {
  ADD_NODE(Ident);
  if (lexer.peek(0) == TokenType::LBRACK) {
    ADD_NODE(TokenNode);
    ADD_NODE(ConstExp);
    EXPECT_TOKEN(TokenType::RBRACK);
  }

  EXPECT_TOKEN(TokenType::ASSIGN);
  ADD_NODE(ConstInitVal);
}

// ConstInitVal ::= ConstExp | '{' [ConstExp {',' ConstExp}] '}'
SIMPLE_PARSE(ConstInitVal) {
  if (lexer.peek(0) == TokenType::LBRACE) {
    ADD_NODE(TokenNode);
    if (lexer.peek(0) != TokenType::RBRACE) {
      ADD_NODE(ConstExp);
      while (lexer.peek(0) == TokenType::COMMA) {
        ADD_NODE(TokenNode);
        ADD_NODE(ConstExp);
      }
    }
    EXPECT_TOKEN(TokenType::RBRACE);
  } else {
    ADD_NODE(ConstExp);
  }
}

// VarDecl      ::= ['static'] BType VarDef {',' VarDef} ';'
SIMPLE_PARSE(VarDecl) {
  if (lexer.peek(0) == TokenType::STATICTK) {
    ADD_NODE(TokenNode);
  }

  ADD_NODE(BType);
  ADD_NODE(VarDef);

  while (lexer.peek(0) == TokenType::COMMA) {
    ADD_NODE(TokenNode);
    ADD_NODE(VarDef);
  }

  EXPECT_TOKEN(TokenType::SEMICN);
}

// VarDef       ::= Ident [ '[' ConstExp ']' ] ['=' InitVal]
SIMPLE_PARSE(VarDef) {
  ADD_NODE(Ident);
  if (lexer.peek(0) == TokenType::LBRACK) {
    ADD_NODE(TokenNode);
    ADD_NODE(ConstExp);
    EXPECT_TOKEN(TokenType::RBRACK);
  }

  if (lexer.peek(0) == TokenType::ASSIGN) {
    ADD_NODE(TokenNode);
    ADD_NODE(InitVal);
  }
}

// InitVal      ::= Exp | '{' [Exp {',' Exp}] '}'
SIMPLE_PARSE(InitVal) {
  if (lexer.peek(0) == TokenType::LBRACE) {
    ADD_NODE(TokenNode);
    if (lexer.peek(0) != TokenType::RBRACE) {
      ADD_NODE(Exp);
      while (lexer.peek(0) == TokenType::COMMA) {
        ADD_NODE(TokenNode);
        ADD_NODE(Exp);
      }
    }
    EXPECT_TOKEN(TokenType::RBRACE);
  } else {
    ADD_NODE(Exp);
  }
}

// FuncDef      ::= FuncType Ident '(' [FuncFParams] ')' Block
SIMPLE_PARSE(FuncDef) {
  ADD_NODE(FuncType);
  ADD_NODE(Ident);
  EXPECT_TOKEN(TokenType::LPARENT);

  // FuncFParams' first token must be a BType
  if (is_btype(lexer.peek(0).get_type())) {
    ADD_NODE(FuncFParams);
  }
  EXPECT_TOKEN(TokenType::RPARENT);
  ADD_NODE(Block);
}

// MainFuncDef  ::= 'int' 'main' '(' ')' Block
SIMPLE_PARSE(MainFuncDef) {
  EXPECT_TOKEN(TokenType::INTTK);
  EXPECT_TOKEN(TokenType::MAINTK);
  EXPECT_TOKEN(TokenType::LPARENT);
  EXPECT_TOKEN(TokenType::RPARENT);
  ADD_NODE(Block);
}

// FuncType     ::= 'void' | 'int'
SIMPLE_PARSE(FuncType) {
  switch (lexer.peek(0).get_type()) {
    case TokenType::VOIDTK:
    case TokenType::INTTK:
      ADD_NODE(TokenNode);
      break;
    default:
      throw std::runtime_error("invalid FuncType token");
  }
}

// FuncFParams  ::= FuncFParam {',' FuncFParam}
SIMPLE_PARSE(FuncFParams) {
  ADD_NODE(FuncFParam);
  while (lexer.peek(0) == TokenType::COMMA) {
    ADD_NODE(TokenNode);
    ADD_NODE(FuncFParam);
  }
}

// FuncFParam   ::= BType Ident [ '[' ']' ]
SIMPLE_PARSE(FuncFParam) {
  ADD_NODE(BType);
  ADD_NODE(Ident);
  if (lexer.peek(0) == TokenType::LBRACK) {
    ADD_NODE(TokenNode);
    EXPECT_TOKEN(TokenType::RBRACK);
  }
}

// Block        ::= '{' {BlockItem} '}'
SIMPLE_PARSE(Block) {
  EXPECT_TOKEN(TokenType::LBRACE);
  while (lexer.peek(0) != TokenType::RBRACE) {
    ADD_NODE(BlockItem);
  }
  EXPECT_TOKEN(TokenType::RBRACE);
}

// BlockItem    ::= Decl | Stmt
SIMPLE_PARSE(BlockItem) {
  if (is_decl(lexer.peek(0).get_type())) {
    ADD_NODE(Decl);
  } else {
    ADD_NODE(Stmt);
  }
}

// ===================== Stmt start =====================

/*
Stmt         ::= LVal '=' Exp ';'
               | [Exp] ';'
               | Block
               | 'if' '(' Cond ')' Stmt ['else' Stmt]
               | 'for' '(' [ForStmt] ';' [Cond] ';' [ForStmt] ')' Stmt
               | 'break' ';'
               | 'continue' ';'
               | 'return' [Exp] ';'
               | 'printf' '(' StringConst {',' Exp} ')' ';'
*/
SIMPLE_PARSE(Stmt) {
  switch (lexer.peek(0).get_type()) {
    case TokenType::LBRACE:
      parse_block(lexer);
      break;
    case TokenType::IFTK:
      parse_if(lexer);
      break;
    case TokenType::FORTK:
      parse_for(lexer);
      break;
    case TokenType::BREAKTK:
      parse_break(lexer);
      break;
    case TokenType::CONTINUETK:
      parse_continue(lexer);
      break;
    case TokenType::RETURNTK:
      parse_return(lexer);
      break;
    case TokenType::PRINTFTK:
      parse_printf(lexer);
      break;
    case TokenType::SEMICN:
      stmt_type = StmtType::EXP;
      ADD_NODE(TokenNode);
      break;
    default:
      lexer.branch();
      ec.disable();
      auto exp = std::make_unique<Exp>();
      exp->parse(lexer);
      ec.enable();

      if (lexer.peek(0) == TokenType::ASSIGN) {
        lexer.rollback();
        parse_assign(lexer);
      } else {
        lexer.rollback();
        parse_exp(lexer);
      }
      break;
  }
}

void Stmt::parse_assign(Lexer& lexer) {
  stmt_type = StmtType::ASSIGN;
  ADD_NODE(LVal);
  EXPECT_TOKEN(TokenType::ASSIGN);
  ADD_NODE(Exp);
  EXPECT_TOKEN(TokenType::SEMICN);
}

void Stmt::parse_exp(Lexer& lexer) {
  stmt_type = StmtType::EXP;
  // Exp's first token must be a unary exp
  if (is_unary_exp(lexer.peek(0).get_type())) {
    ADD_NODE(Exp);
  }
  EXPECT_TOKEN(TokenType::SEMICN);
}

void Stmt::parse_block(Lexer& lexer) {
  stmt_type = StmtType::BLOCK;
  ADD_NODE(Block);
}

void Stmt::parse_if(Lexer& lexer) {
  stmt_type = StmtType::IF;
  EXPECT_TOKEN(TokenType::IFTK);
  EXPECT_TOKEN(TokenType::LPARENT);
  ADD_NODE(Cond);
  EXPECT_TOKEN(TokenType::RPARENT);
  ADD_NODE(Stmt);

  if (lexer.peek(0) == TokenType::ELSETK) {
    ADD_NODE(TokenNode);
    ADD_NODE(Stmt);
  }
}

void Stmt::parse_for(Lexer& lexer) {
  stmt_type = StmtType::FOR;
  EXPECT_TOKEN(TokenType::FORTK);
  EXPECT_TOKEN(TokenType::LPARENT);
  if (lexer.peek(0) != TokenType::SEMICN) {
    ADD_NODE(ForStmt);
  }
  EXPECT_TOKEN(TokenType::SEMICN);
  if (lexer.peek(0) != TokenType::SEMICN) {
    ADD_NODE(Cond);
  }
  EXPECT_TOKEN(TokenType::SEMICN);
  if (lexer.peek(0) != TokenType::RPARENT) {
    ADD_NODE(ForStmt);
  }
  EXPECT_TOKEN(TokenType::RPARENT);
  ADD_NODE(Stmt);
}

void Stmt::parse_break(Lexer& lexer) {
  stmt_type = StmtType::BREAK;
  EXPECT_TOKEN(TokenType::BREAKTK);
  EXPECT_TOKEN(TokenType::SEMICN);
}

void Stmt::parse_continue(Lexer& lexer) {
  stmt_type = StmtType::CONTINUE;
  EXPECT_TOKEN(TokenType::CONTINUETK);
  EXPECT_TOKEN(TokenType::SEMICN);
}

void Stmt::parse_return(Lexer& lexer) {
  stmt_type = StmtType::RETURN;
  EXPECT_TOKEN(TokenType::RETURNTK);
  // Exp's first token must be a unary exp
  if (is_unary_exp(lexer.peek(0).get_type())) {
    ADD_NODE(Exp);
  }
  EXPECT_TOKEN(TokenType::SEMICN);
}

void Stmt::parse_printf(Lexer& lexer) {
  stmt_type = StmtType::PRINTF;
  EXPECT_TOKEN(TokenType::PRINTFTK);
  EXPECT_TOKEN(TokenType::LPARENT);
  EXPECT_TOKEN(TokenType::STRCON);
  while (lexer.peek(0) == TokenType::COMMA) {
    ADD_NODE(TokenNode);
    ADD_NODE(Exp);
  }
  EXPECT_TOKEN(TokenType::RPARENT);
  EXPECT_TOKEN(TokenType::SEMICN);
}
// ===================== Stmt end ======================

// ForStmt      ::= LVal '=' Exp {',' LVal '=' Exp}
SIMPLE_PARSE(ForStmt) {
  ADD_NODE(LVal);
  EXPECT_TOKEN(TokenType::ASSIGN);
  ADD_NODE(Exp);

  while (lexer.peek(0) == TokenType::COMMA) {
    ADD_NODE(TokenNode);
    ADD_NODE(LVal);
    EXPECT_TOKEN(TokenType::ASSIGN);
    ADD_NODE(Exp);
  }
}

// Exp          ::= AddExp
SIMPLE_PARSE(Exp) { ADD_NODE(AddExp); }

// Cond         ::= LOrExp
SIMPLE_PARSE(Cond) { ADD_NODE(LOrExp); }

// LVal         ::= Ident [ '[' Exp ']' ]
SIMPLE_PARSE(LVal) {
  ADD_NODE(Ident);
  if (lexer.peek(0) == TokenType::LBRACK) {
    ADD_NODE(TokenNode);
    ADD_NODE(Exp);
    EXPECT_TOKEN(TokenType::RBRACK);
  }
}

// PrimaryExp   ::= '(' Exp ')' | LVal | Number
SIMPLE_PARSE(PrimaryExp) {
  if (lexer.peek(0) == TokenType::LPARENT) {
    ADD_NODE(TokenNode);
    ADD_NODE(Exp);
    EXPECT_TOKEN(TokenType::RPARENT);
  } else if (lexer.peek(0) == TokenType::IDENFR) {
    ADD_NODE(LVal);
  } else {
    ADD_NODE(Number);
  }
}

// Number       ::= IntConst
SIMPLE_PARSE(Number) { EXPECT_TOKEN(TokenType::INTCON); }

// UnaryExp     ::= PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
SIMPLE_PARSE(UnaryExp) {
  if (lexer.peek(0) == TokenType::IDENFR &&
      lexer.peek(1) == TokenType::LPARENT) {
    ADD_NODE(Ident);
    EXPECT_TOKEN(TokenType::LPARENT);
    // FuncRParams' first token must be an Exp(UnaryExp)
    if (is_unary_exp(lexer.peek(0).get_type())) {
      ADD_NODE(FuncRParams);
    }
    EXPECT_TOKEN(TokenType::RPARENT);
  } else if (is_unary_op(lexer.peek(0).get_type())) {
    ADD_NODE(UnaryOp);
    ADD_NODE(UnaryExp);
  } else {
    ADD_NODE(PrimaryExp);
  }
}

// UnaryOp      ::= '+' | '-' | '!'
SIMPLE_PARSE(UnaryOp) {
  switch (lexer.peek(0).get_type()) {
    case TokenType::PLUS:
    case TokenType::MINU:
    case TokenType::NOT:
      ADD_NODE(TokenNode);
      break;
    default:
      throw std::runtime_error("invalid UnaryOp token");
  }
}

// FuncRParams  ::= Exp {',' Exp}
SIMPLE_PARSE(FuncRParams) {
  ADD_NODE(Exp);
  while (lexer.peek(0) == TokenType::COMMA) {
    ADD_NODE(TokenNode);
    ADD_NODE(Exp);
  }
}

// MulExp       ::= UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
SIMPLE_PARSE(MulExp) {
  ADD_NODE(UnaryExp);
  while (lexer.peek(0) == TokenType::MULT || lexer.peek(0) == TokenType::DIV ||
         lexer.peek(0) == TokenType::MOD) {
    ADD_NODE(TokenNode);
    ADD_NODE(UnaryExp);
  }
}

// AddExp       ::= MulExp | AddExp ('+' | '-') MulExp
SIMPLE_PARSE(AddExp) {
  ADD_NODE(MulExp);
  while (lexer.peek(0) == TokenType::PLUS || lexer.peek(0) == TokenType::MINU) {
    ADD_NODE(TokenNode);
    ADD_NODE(MulExp);
  }
}

// RelExp       ::= AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
SIMPLE_PARSE(RelExp) {
  ADD_NODE(AddExp);
  while (lexer.peek(0) == TokenType::LSS || lexer.peek(0) == TokenType::GRE ||
         lexer.peek(0) == TokenType::LEQ || lexer.peek(0) == TokenType::GEQ) {
    ADD_NODE(TokenNode);
    ADD_NODE(AddExp);
  }
}

// EqExp        ::= RelExp | EqExp ('==' | '!=') RelExp
SIMPLE_PARSE(EqExp) {
  ADD_NODE(RelExp);
  while (lexer.peek(0) == TokenType::EQL || lexer.peek(0) == TokenType::NEQ) {
    ADD_NODE(TokenNode);
    ADD_NODE(RelExp);
  }
}

// LAndExp      ::= EqExp | LAndExp '&&' EqExp
SIMPLE_PARSE(LAndExp) {
  ADD_NODE(EqExp);
  while (lexer.peek(0) == TokenType::AND) {
    ADD_NODE(TokenNode);
    ADD_NODE(EqExp);
  }
}

// LOrExp       ::= LAndExp | LOrExp '||' LAndExp
SIMPLE_PARSE(LOrExp) {
  ADD_NODE(LAndExp);
  while (lexer.peek(0) == TokenType::OR) {
    ADD_NODE(TokenNode);
    ADD_NODE(LAndExp);
  }
}

// ConstExp     ::= AddExp
SIMPLE_PARSE(ConstExp) { ADD_NODE(AddExp); }

SIMPLE_PARSE(Ident) { EXPECT_TOKEN(TokenType::IDENFR); }

#undef EXPECT_TOKEN
#undef ADD_NODE
#undef SIMPLE_PARSE

}  // namespace frontend::ast

namespace frontend::ast {

#define DEFINE_ACCEPT(className)                                    \
  void className::accept(midend::visitor::Visitor& visitor) const { \
    visitor.visit(*this);                                           \
  }

#define X(_, name) DEFINE_ACCEPT(name)
NODE_TABLE
#undef X

#undef DEFINE_ACCEPT

void Node::output_ast(std::ostream& os) const {
  for (const auto& child : children) {
    child->output_ast(os);
  }
  if (print_self) {
    os << '<' << type << ">\n";
  }
}

void RecurNode::output_ast(std::ostream& os) const {
  for (const auto& child : children) {
    child->output_ast(os);
    if (child->get_type() != NodeType::TOKEN) {
      if (print_self) os << "<" << type << ">\n";
    }
  }
}

void TokenNode::output_ast(std::ostream& os) const { os << token << '\n'; }

}  // namespace frontend::ast
