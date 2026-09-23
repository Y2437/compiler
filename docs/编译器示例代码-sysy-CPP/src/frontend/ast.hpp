#ifndef BUAA_COMPILER_AST
#define BUAA_COMPILER_AST

#include <memory>
#include <ostream>
#include <vector>

#include "frontend/lexer.hpp"
#include "frontend/token.hpp"

namespace midend::visitor {
class Visitor;
}

namespace frontend::ast {
#define NODE_TABLE                \
  X(COMP_UNIT, CompUnit)          \
  X(DECL, Decl)                   \
  X(CONST_DECL, ConstDecl)        \
  X(BTYPE, BType)                 \
  X(CONST_DEF, ConstDef)          \
  X(CONST_INIT_VAL, ConstInitVal) \
  X(VAR_DECL, VarDecl)            \
  X(VAR_DEF, VarDef)              \
  X(INIT_VAL, InitVal)            \
  X(FUNC_DEF, FuncDef)            \
  X(MAIN_FUNC_DEF, MainFuncDef)   \
  X(FUNC_TYPE, FuncType)          \
  X(FUNC_F_PARAMS, FuncFParams)   \
  X(FUNC_F_PARAM, FuncFParam)     \
  X(BLOCK, Block)                 \
  X(BLOCK_ITEM, BlockItem)        \
  X(STMT, Stmt)                   \
  X(FOR_STMT, ForStmt)            \
  X(EXP, Exp)                     \
  X(COND, Cond)                   \
  X(LVAL, LVal)                   \
  X(PRIMARY_EXP, PrimaryExp)      \
  X(NUMBER, Number)               \
  X(UNARY_EXP, UnaryExp)          \
  X(UNARY_OP, UnaryOp)            \
  X(FUNC_R_PARAMS, FuncRParams)   \
  X(MUL_EXP, MulExp)              \
  X(ADD_EXP, AddExp)              \
  X(REL_EXP, RelExp)              \
  X(EQ_EXP, EqExp)                \
  X(LAND_EXP, LAndExp)            \
  X(LOR_EXP, LOrExp)              \
  X(CONST_EXP, ConstExp)          \
  X(TOKEN, TokenNode)             \
  X(IDENT, Ident)
}  // namespace frontend::ast

namespace frontend::ast {
using lexer::Lexer;
using token::Token;
using token::TokenType;

enum class NodeType {
#define X(enum, name) enum,
  NODE_TABLE
#undef X
      COUNT,
};
std::ostream& operator<<(std::ostream& os, NodeType type);

class Node;

#define X(_, name) class name;
NODE_TABLE
#undef X

using NodePtr = std::unique_ptr<Node>;

class Node {
 public:
  // some nodes like TokenNode don't need to print themselves
  explicit Node(NodeType type, bool printSelf = false)
      : type(type), print_self(printSelf) {}
  virtual ~Node() = default;
  virtual void parse(Lexer& lexer) = 0;
  virtual void accept(midend::visitor::Visitor& visitor) const = 0;
  virtual void output_ast(std::ostream& os) const;

  void add_child(NodePtr node) { children.emplace_back(std::move(node)); }
  auto get_type() const -> NodeType { return type; }
  auto get_children() const -> const std::vector<NodePtr>& { return children; }
  void add_node(NodePtr node, Lexer& lexer) {
    node->parse(lexer);
    children.emplace_back(std::move(node));
  }

 protected:
  const NodeType type;
  const bool print_self;
  std::vector<NodePtr> children;
};

class RecurNode : public Node {
 public:
  explicit RecurNode(NodeType type, bool printSelf = false)
      : Node(type, printSelf) {}
  void output_ast(std::ostream& os) const final;
};

// clang-format off
#define SIMPLE_NODE(enumName, className, print_self)                  \
    class className : public Node {                                   \
    public:                                                           \
        className() : Node(NodeType::enumName, print_self) {}         \
        void accept(midend::visitor::Visitor &visitor) const override; \
        void parse(Lexer &lexer) override;                            \
    }

#define SIMPLE_RECUR(enumName, className, print_self)                 \
    class className : public RecurNode {                              \
    public:                                                           \
        className() : RecurNode(NodeType::enumName, print_self) {}    \
        void accept(midend::visitor::Visitor &visitor) const override; \
        void parse(Lexer &lexer) override;                            \
    }

// ========== Node Definitions ==========

class TokenNode : public Node {
public:
    TokenNode() : Node(NodeType::TOKEN, false) {}
    TokenNode(Token&& token) : Node(NodeType::TOKEN, false), token(std::move(token)) {}
    void accept(midend::visitor::Visitor &visitor) const override;
    void parse(Lexer &lexer) override;
    void output_ast(std::ostream &os) const override;
    Token get_token() const { return token; }
private:
    Token token;
};

SIMPLE_NODE(COMP_UNIT, CompUnit, true);
SIMPLE_NODE(DECL, Decl, false);
SIMPLE_NODE(CONST_DECL, ConstDecl, true);
SIMPLE_NODE(BTYPE, BType, false);
SIMPLE_NODE(CONST_DEF, ConstDef, true);
SIMPLE_NODE(CONST_INIT_VAL, ConstInitVal, true);
SIMPLE_NODE(VAR_DECL, VarDecl, true);
SIMPLE_NODE(VAR_DEF, VarDef, true);
SIMPLE_NODE(INIT_VAL, InitVal, true);
SIMPLE_NODE(FUNC_DEF, FuncDef, true);
SIMPLE_NODE(MAIN_FUNC_DEF, MainFuncDef, true);
SIMPLE_NODE(FUNC_TYPE, FuncType, true);
SIMPLE_NODE(FUNC_F_PARAMS, FuncFParams, true);
SIMPLE_NODE(FUNC_F_PARAM, FuncFParam, true);
SIMPLE_NODE(BLOCK, Block, true);
SIMPLE_NODE(BLOCK_ITEM, BlockItem, false);
class Stmt : public Node {
public:
    Stmt() : Node(NodeType::STMT, true) {}
    void accept(midend::visitor::Visitor &visitor) const override;
    void parse(Lexer &lexer) override;
    enum class StmtType {
        ASSIGN,
        EXP,
        BLOCK,
        IF,
        FOR,
        BREAK,
        CONTINUE,
        RETURN,
        PRINTF,
    };
    StmtType get_stmt_type() const { return stmt_type; }
private:
    StmtType stmt_type;
    void parse_assign(Lexer &lexer);
    void parse_exp(Lexer &lexer);
    void parse_block(Lexer &lexer);
    void parse_if(Lexer &lexer);
    void parse_for(Lexer &lexer);
    void parse_break(Lexer &lexer);
    void parse_continue(Lexer &lexer);
    void parse_return(Lexer &lexer);
    void parse_printf(Lexer &lexer);
};
SIMPLE_NODE(FOR_STMT, ForStmt, true);
SIMPLE_NODE(NUMBER, Number, true);
SIMPLE_NODE(UNARY_OP, UnaryOp, true);
SIMPLE_NODE(FUNC_R_PARAMS, FuncRParams, true);
SIMPLE_NODE(IDENT, Ident, false);

SIMPLE_NODE(EXP, Exp, true);
SIMPLE_NODE(COND, Cond, true);
SIMPLE_NODE(LVAL, LVal, true);
SIMPLE_NODE(PRIMARY_EXP, PrimaryExp, true);
SIMPLE_NODE(UNARY_EXP, UnaryExp, true);
SIMPLE_NODE(CONST_EXP, ConstExp, true);

// ========== Recur Nodes ==========

SIMPLE_RECUR(MUL_EXP, MulExp, true);
SIMPLE_RECUR(ADD_EXP, AddExp, true);
SIMPLE_RECUR(REL_EXP, RelExp, true);
SIMPLE_RECUR(EQ_EXP, EqExp, true);
SIMPLE_RECUR(LAND_EXP, LAndExp, true);
SIMPLE_RECUR(LOR_EXP, LOrExp, true);

#undef SIMPLE_RECUR
#undef SIMPLE_NODE
// clang-format on

}  // namespace frontend::ast

#endif
