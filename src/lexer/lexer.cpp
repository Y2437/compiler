#include "lexer.h"
using namespace std;
#define ui unsigned int 
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


// 保留字表
map <string, LexTokenKind> KEYWORDS = {
    {"while", WHILETK}, {"case", CASETK}, {"char", CHARTK}, {"else", ELSETK},
    {"continue", CONTINUETK}, {"main", MAINTK}, {"default", DEFAULTTK}, {"void", VOIDTK},
    {"printf", PRINTFTK}, {"int", INTTK}, {"return", RETURNTK}, {"if", IFTK},
    {"static", STATICTK}, {"break", BREAKTK}, {"switch", SWITCHTK}, {"const", CONSTTK}
};



// 枚举量名称表

const char *LexTokenKindName(LexTokenKind kind) {
    #define X(name,num) [num]=#name,
    static const char * tokenName[]={
        #include "../config/lexer_token.def"  
    };
    #undef X
    if(kind<0||kind>=LTK_COUNT) return "Error";
    return tokenName[kind];
}





 Lexer::Lexer(ErrorController * errorControler, vector<LexToken> & tokenList) :
    tokenList(tokenList),
    curPos(0),
    lineNum(1),
    number(0),
    isDoubleOperator(false),
    tokenType(Eof),
    errorControler(errorControler)
{
    source.clear();
    token.clear();
}

void Lexer::read_source(FILE * in){
    int ch=fgetc(in);
    while (ch!=EOF)
    {
        source.push_back(ch);
        ch=fgetc(in);
    }
}
void Lexer::scan(){
    while (1)
    {
        move_until_nbc();
        if(curPos>=source.length()) break;
        int ch=get_cur();
        int nt=get_next();
        reset_token();
        startLineNum=lineNum;
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
void Lexer::report_token_list(FILE * lep){
    for (auto & it : tokenList)
    {
        fprintf(lep,"%s %s\n",LexTokenKindName(it.tokenType),it.raw_string.c_str());
    }
}


int Lexer::get_cur(){
    if(curPos>=source.size()){
        return EOF;
    }
    return source[curPos];
}
void Lexer::read_one_char(){
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
void Lexer::move_forward(){     //注意维护行号
    if(curPos<source.size()) {
        if(get_cur()=='\n') lineNum++;
        curPos++;
    }

}
void Lexer::move_backward(){   //对称!
    if(curPos>0) {
        curPos--;
        if(get_cur()=='\n') lineNum--;
    }
}
int Lexer::get_next(){
    if(curPos<source.size()-1){
        return source[curPos+1];
    }else return EOF;
}
void Lexer::move_until_nbc(){       //调用完这个之后的get_cur必定返回一个非空白字符或EOF
    if(get_cur()==EOF) return;
    while(isspace(get_cur())){
        move_forward();
    }
}
void Lexer::reset_token(){
    token.clear();
}
LexTokenKind Lexer::get_operator_id(){
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
            errorControler->register_error(LEX_INVALID_TOKEN,lineNum);
            return LexError;   //哎,可惜不优雅,但是不改会warn
        default: return LexError;
    }
}
LexToken Lexer::get_operator_token(){
    LexTokenKind kind=get_operator_id();
    if(isDoubleOperator){
        token.push_back(get_cur());
        move_forward();
    }
    token.push_back(get_cur());
    move_forward();
    return LexToken{kind,token,startLineNum};
}
LexToken Lexer::get_quoted_token(char quote,LexTokenKind kind){
    token.push_back(get_cur());
    move_forward();
    while (get_cur()!=quote&&get_cur()!=EOF)
    {
        read_one_char();
    }
    token.push_back(get_cur());
    move_forward();
    return LexToken{kind,token,startLineNum};
}
LexToken Lexer::get_const_str_token(){
    return get_quoted_token('\"',STRCON);
}

LexToken Lexer::get_const_char_token(){
    return get_quoted_token('\'',CHARCON);
}

LexToken Lexer::get_const_int_token(){
    int ch=get_cur();
    while(isdigit(ch)){
        token.push_back(ch);
        move_forward();
        ch=get_cur();
    }
    return LexToken{INTCON,token,startLineNum};
}

LexToken Lexer::get_ident_or_revserve_token(){
    int ch=get_cur();
    while (isdigit(ch)||isalpha(ch)||ch=='_')
    {
        token.push_back(ch);
        move_forward();
        ch=get_cur();
    }
    if(auto it = KEYWORDS.find(token); it != KEYWORDS.end()){
        return LexToken{it->second,it->first,startLineNum};  //枚举值,内容
    }
    return LexToken{IDENFR,token,startLineNum};
}
void Lexer::skip_line(){
    int ch=get_cur();
    while (ch!=EOF&&ch!='\n')
    {
        move_forward();
        ch=get_cur();
    }
} 
void Lexer::skip_block(){
    int c1=get_cur();
    int c2=get_next();
    move_forward();   // 防止/*/的小巧思
    while (c1!=EOF&&c2!=EOF&&!(c1=='*'&&c2=='/'))
    {
        move_forward();
        c1=get_cur();
        c2=get_next();
    }  
    move_forward(); //得等它结束啊
    move_forward();
}

