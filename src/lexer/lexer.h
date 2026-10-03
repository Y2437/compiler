#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include "../error/errorController.h"
#include "../config/enums.h"
#include "../config/structs.h"
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace std;
// ================== 种别码表(编号 / 枚举名 / 单词名称) ==================
//
//   0    Eof              文件结束(EOF)
//
//   1 ~ 16   保留字段
//   1    WHILETK     "while"     2    CASETK      "case"      3    CHARTK      "char"
//   4    ELSETK      "else"      5    CONTINUETK  "continue"  6    MAINTK      "main"
//   7    DEFAULTTK   "default"   8    VOIDTK      "void"      9    PRINTFTK    "printf"
//   10   INTTK       "int"       11   RETURNTK    "return"    12   IFTK        "if"
//   13   STATICTK    "static"    14   BREAKTK     "break"     15   SWITCHTK    "switch"
//   16   CONSTTK     "const"
//
//   17   IDENFR      标识符
//   18   INTCON      整型常数
//   19   CHARCON     字符常数
//   20   STRCON      字符串常数
//
//   21 ~ 30  单字符运算符段
//   21   PLUS      "+"      22   MINU      "-"      23   MULT      "*"      24   DIV       "/"
//   25   MOD       "%"      26   LSS       "<"      27   GRE       ">"      28   ASSIGN    "="
//   29   NOT       "!"      30   COLON     ":"
//
//   31 ~ 36  双字符运算符段
//   31   GEQ       ">="     32   LEQ       "<="     33   EQL       "=="     34   NEQ       "!="
//   35   AND       "&&"     36   OR        "||"
//
//   37 ~ 44  界符段
//   37   LPARENT   "("      38   RPARENT   ")"      39   LBRACK    "["      40   RBRACK    "]"
//   41   LBRACE    "{"      42   RBRACE    "}"      43   COMMA     ","      44   SEMICN    ";"
//
//   45   Error            错误(无法识别的字符)
//
// ======================================================================







extern map<string, LexTokenKind> KEYWORDS;

const char *LexTokenKindName(LexTokenKind kind);

class Lexer {
public:
    // source：源程序字符串
    // curPos：当前字符串位置指针
    // token：解析单词值
    // tokenType：解析单词类型
    // lineNum：当前行号
    // number：解析数值

    Lexer(ErrorController *errorControler,vector<LexToken> & tokenList);
    void read_source(FILE * in);
    void scan();
    void report_token_list(FILE *lep);

private:
    size_t curPos;
    int lineNum;
    int startLineNum;
    int number;
    bool isDoubleOperator;
    string token;
    LexTokenKind tokenType;
    string source;
    ErrorController *errorControler;
    vector<LexToken> & tokenList;
    int get_cur();
    int get_next();
    void read_one_char();
    void move_forward();
    void move_backward();
    void move_until_nbc();
    void reset_token();
    LexTokenKind get_operator_id();
    LexToken get_operator_token();
    LexToken get_quoted_token(char quote, LexTokenKind kind);
    LexToken get_const_str_token();
    LexToken get_const_char_token();
    LexToken get_const_int_token();
    LexToken get_ident_or_revserve_token();
    void skip_line();
    void skip_block();
};

#endif 