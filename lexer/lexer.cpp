#include <bits/stdc++.h>
using namespace std;
#define ll long long
#define ull unsigned long long
#define ui unsigned int 
#define TEST_INT(A)  printf("------>%d\n",A)
#define PS putchar(32)
#define NL putchar(10)
// ================== 种别码表(编码 / 类别码 / 单词名称) ==================
//
//   0    Eof              文件结束(EOF)
//   -1   Error            错误(无法识别的字符)
//
//   1 ~ 99  保留字段
//   1    WHILETK     "while"     2    CASETK      "case"      3    CHARTK      "char"
//   4    ELSETK      "else"      5    CONTINUETK  "continue"  6    MAINTK      "main"
//   7    DEFAULTTK   "default"   8    VOIDTK      "void"      9    PRINTFTK    "printf"
//   10   INTTK       "int"       11   RETURNTK    "return"    12   IFTK        "if"
//   13   STATICTK    "static"    14   BREAKTK     "break"     15   SWITCHTK    "switch"
//   16   CONSTTK     "const"
//
//   101  IDENFR      标识符
//   102  INTCON      整型常数
//   103  CHARCON     字符常数
//   104  STRCON      字符串常数
//
//   201 ~ 250  单字符运算符段
//   201  PLUS      "+"      202  MINU      "-"      203  MULT      "*"      204  DIV       "/"
//   205  MOD       "%"      206  LSS       "<"      207  GRE       ">"      208  ASSIGN    "="
//   209  NOT       "!"      210  COLON     ":"
//
//   251 ~ 300  双字符运算符段
//   251  GEQ       ">="     252  LEQ       "<="     253  EQL       "=="     254  NEQ       "!="
//   255  AND       "&&"     256  OR        "||"
//
//   301 ~ 399  界符段
//   301  LPARENT   "("      302  RPARENT   ")"      303  LBRACK    "["      304  RBRACK    "]"
//   305  LBRACE    "{"      306  RBRACE    "}"      307  COMMA     ","      308  SEMICN    ";"
//
// ======================================================================
//Token类别
enum TokenKind {
    Eof = 0,
    Error = -1,
    // 保留字
    WHILETK = 1, CASETK = 2, CHARTK = 3, ELSETK = 4,
    CONTINUETK = 5, MAINTK = 6, DEFAULTTK = 7, VOIDTK = 8,
    PRINTFTK = 9, INTTK = 10, RETURNTK = 11, IFTK = 12,
    STATICTK = 13, BREAKTK = 14, SWITCHTK = 15, CONSTTK = 16,
    // 标识符与常数
    IDENFR = 101, INTCON = 102, CHARCON = 103, STRCON = 104,
    // 单字符运算符
    PLUS = 201, MINU = 202, MULT = 203, DIV = 204,
    MOD = 205, LSS = 206, GRE = 207, ASSIGN = 208,
    NOT = 209, COLON = 210,
    // 双字符运算符
    GEQ = 251, LEQ = 252, EQL = 253, NEQ = 254, AND = 255, OR = 256,
    // 界符
    LPARENT = 301, RPARENT = 302, LBRACK = 303, RBRACK = 304,
    LBRACE = 305, RBRACE = 306, COMMA = 307, SEMICN = 308
};
// 保留字表
map <string, TokenKind> KEYWORDS = {
    {"while", WHILETK}, {"case", CASETK}, {"char", CHARTK}, {"else", ELSETK},
    {"continue", CONTINUETK}, {"main", MAINTK}, {"default", DEFAULTTK}, {"void", VOIDTK},
    {"printf", PRINTFTK}, {"int", INTTK}, {"return", RETURNTK}, {"if", IFTK},
    {"static", STATICTK}, {"break", BREAKTK}, {"switch", SWITCHTK}, {"const", CONSTTK}
};
// 枚举量名称表
const char* tokenKindName(TokenKind kind) {
    switch (kind) {
        // 文件结束与错误
        case Eof:            return "Eof";
        case Error:          return "Error";
        // 保留字
        case WHILETK:        return "WHILETK";
        case CASETK:         return "CASETK";
        case CHARTK:         return "CHARTK";
        case ELSETK:         return "ELSETK";
        case CONTINUETK:     return "CONTINUETK";
        case MAINTK:         return "MAINTK";
        case DEFAULTTK:      return "DEFAULTTK";
        case VOIDTK:         return "VOIDTK";
        case PRINTFTK:       return "PRINTFTK";
        case INTTK:          return "INTTK";
        case RETURNTK:       return "RETURNTK";
        case IFTK:           return "IFTK";
        case STATICTK:       return "STATICTK";
        case BREAKTK:        return "BREAKTK";
        case SWITCHTK:       return "SWITCHTK";
        case CONSTTK:        return "CONSTTK";
        // 标识符与常数
        case IDENFR:         return "IDENFR";
        case INTCON:         return "INTCON";
        case CHARCON:        return "CHARCON";
        case STRCON:         return "STRCON";
        // 单字符运算符
        case PLUS:           return "PLUS";
        case MINU:           return "MINU";
        case MULT:           return "MULT";
        case DIV:            return "DIV";
        case MOD:            return "MOD";
        case LSS:            return "LSS";
        case GRE:            return "GRE";
        case ASSIGN:         return "ASSIGN";
        case NOT:            return "NOT";
        case COLON:          return "COLON";
        // 双字符运算符
        case GEQ:            return "GEQ";
        case LEQ:            return "LEQ";
        case EQL:            return "EQL";
        case NEQ:            return "NEQ";
        case AND:            return "AND";
        case OR:             return "OR";
        // 界符
        case LPARENT:        return "LPARENT";
        case RPARENT:        return "RPARENT";
        case LBRACK:         return "LBRACK";
        case RBRACK:         return "RBRACK";
        case LBRACE:         return "LBRACE";
        case RBRACE:         return "RBRACE";
        case COMMA:          return "COMMA";
        case SEMICN:         return "SEMICN";
        default: return "UNKNOWN";
    }
}
struct Error{
    char type;
    int lineNum;
};

class ErrorControler{
    public:
        vector<struct Error> errorList;
        void register_error(char type,int lineNum ){
            errorList.push_back({type,lineNum});
        }
};
struct Token
{
    TokenKind tokenType;
    string raw_string;
};

class Lexer{
    // source：源程序字符串
    // curPos：当前字符串位置指针
    // token：解析单词值
    // tokenType：解析单词类型
    // lineNum：当前行号
    // number：解析数值
    public:
        vector<Token> tokenList;
        Lexer(ErrorControler * errorControler) :   curPos(0) ,
                    lineNum(1),
                    number(0),
                    tokenType(Eof),
                    isDoubleOperator(false),
                    errorControler(errorControler)
                {
                    source.clear();
                    token.clear();
                    tokenList.clear();
                }

        void read_source(){
            int ch=getchar();
            while (ch!=EOF)
            {
                source.push_back(ch);
                ch=getchar();
            }
        }
        void scan(){
            while (1)
            {
                move_until_nbc();
                if(curPos>=source.length()) break;
                int ch=get_cur();
                int nt=get_next();
                reset_token();
                if(ch=='\"'){//STRCON
                    tokenList.push_back(get_const_str_token());
                }else if(ch=='\''){//CHARCON
                    tokenList.push_back(get_const_char_token());
                }else if(isdigit(ch)){ //INTCON
                    tokenList.push_back(get_const_int_token());
                }else if(isalpha(ch)||ch=='_'){  //Ident or KW
                    tokenList.push_back(get_ident_or_revserve_token());
                }else if(ch=='/'&& nt=='/') {//line comment
                    skip_line();
                }else if(ch=='/'&& nt=='*') {//block comment
                    skip_block();
                }else{  //OPERA
                    tokenList.push_back(get_operator_token());
                }
                
            }
        }
        bool has_error(){
            return !errorControler->errorList.empty();
        }
        void report_error(FILE * erp){
            for(auto & it : errorControler->errorList){
                fprintf(erp,"%d %c\n",it.lineNum,it.type);
            }
        }
        void report_lex(FILE * lep){
            for (auto & it : tokenList)
            {
                fprintf(lep,"%s %s\n",tokenKindName(it.tokenType),it.raw_string.c_str());
            }
        }

    private:
        int curPos=0;
        int lineNum=0;
        int number=0;
        bool isDoubleOperator=false;
        string token;
        TokenKind tokenType;
        string source;
        ErrorControler * errorControler;

        int get_cur(){
            if(curPos>=source.size()){
                return EOF;
            }
            return source[curPos];
        }
        void read_one_char(){
            int ch = get_cur();
            if (ch == EOF) return;
            token.push_back(ch);
            move_forward();
            if (ch == '\\') {
                int escaped = get_cur();
                if (escaped == EOF) return;

                token.push_back(escaped);
                move_forward();
            }
        }
        void move_forward(){     //注意维护行号
            if(curPos<source.size()) {
                if(get_cur()=='\n') lineNum++;
                curPos++;
            }

        }
        void move_backward(){   //对称!
            if(curPos>0) {
                curPos--;
                if(get_cur()=='\n') lineNum--;
            }
        }
        int get_next(){
            if(curPos<source.size()-1){
                return source[curPos+1];
            }else return EOF;
        }
        void move_until_nbc(){       //调用完这个之后的get_cur必定返回一个非空白字符或EOF
            if(get_cur()==EOF) return;
            while(isspace(get_cur())){
                move_forward();
            }
        }
        void reset_token(){
            token.clear();
        }
        TokenKind get_operator_id(){
            ui c1=get_cur();
            ui c2=get_next();
            ui key = (c1<<8 | c2);
            isDoubleOperator=true;
            switch (key) {
                case (('=' << 8) | '='): return EQL;   //优雅,永不过时
                case (('<' << 8) | '='): return LEQ;
                case (('>' << 8) | '='): return GEQ;
                case (('!' << 8) | '='): return NEQ;
                case (('&' << 8) | '&'): return AND;
                case (('|' << 8) | '|'): return OR;
            }
            isDoubleOperator=false;
            switch (c1)
            {
                case '+': return PLUS;
                case '-': return MINU;
                case '*': return MULT;
                case '/': return DIV;
                case '%': return MOD;
                case '<': return LSS;
                case '>': return GRE;
                case '=': return ASSIGN;
                case '!': return NOT;
                case ':': return COLON;
                case '(': return LPARENT;
                case ')': return RPARENT;
                case '[': return LBRACK;
                case ']': return RBRACK;
                case '{': return LBRACE;
                case '}': return RBRACE;
                case ',': return COMMA;
                case ';': return SEMICN;
                case '&':
                case '|':
                    errorControler->register_error('a',lineNum);
                default: return Error;
            }
        }
        Token get_operator_token(){
            TokenKind kind=get_operator_id();
            if(isDoubleOperator){
                token.push_back(get_cur());
                move_forward();
            }
            token.push_back(get_cur());
            move_forward();
            return Token{kind,token};
        }
        Token get_quoted_token(char quote,TokenKind kind){
            token.push_back(get_cur());
            move_forward();
            while (get_cur()!=quote&&get_cur()!=EOF)
            {
                read_one_char();
            }
            token.push_back(get_cur());
            move_forward();
            return Token{kind,token};
        }
        Token get_const_str_token(){
            return get_quoted_token('\"',STRCON);
        }

        Token get_const_char_token(){
            return get_quoted_token('\'',CHARCON);
        }

        Token get_const_int_token(){
            int ch=get_cur();
            while(isdigit(ch)){
                token.push_back(ch);
                move_forward();
                ch=get_cur();
            }
            return Token{INTCON,token};
        }

        Token get_ident_or_revserve_token(){
            int ch=get_cur();
            while (isdigit(ch)||isalpha(ch)||ch=='_')
            {
                token.push_back(ch);
                move_forward();
                ch=get_cur();
            }
            if(auto it = KEYWORDS.find(token); it != KEYWORDS.end()){
                return Token{it->second,it->first};  //枚举值,内容
            }
            return Token{IDENFR,token};
        }
        void skip_line(){
            int ch=get_cur();
            while (ch!=EOF&&ch!='\n')
            {
                move_forward();
                ch=get_cur();
            }
        } 
        void skip_block(){
            int c1=get_cur();
            int c2=get_next();
            move_forward();   // 防止/*/的小巧思
            while (c1!=EOF&&c2!=EOF&&!(c1=='*'&&c2=='/'))
            {
                move_forward();
                c1=get_cur();
                c2=get_cur();
            }  
            move_forward(); //得等它结束啊
            move_forward();
        }
        
        

};

int main() {
    freopen("testfile.txt", "r",stdin);
    FILE* erp = fopen("error.txt", "w");
    FILE* lep = fopen("lexer.txt", "w");
    ErrorControler ec=ErrorControler();
    Lexer lexer=Lexer(&ec);
    lexer.read_source();
    lexer.scan();
    if(lexer.has_error()){
        lexer.report_error(erp);
    }else{
        lexer.report_lex(lep);
    }
    return 0;
}
