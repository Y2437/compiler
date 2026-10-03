// 编译单元 CompUnit → {Decl} {FuncDef} MainFuncDef
// 声明 Decl → ConstDecl | VarDecl
// 常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';' // i
// 基本类型 BType → 'int' | 'char'
// 常量定义 ConstDef → Ident [ '[' ConstExp ']' ] '=' ConstInitVal // k
// 常量初值 ConstInitVal → ConstExp | '{' [ ConstExp { ',' ConstExp } ] '}' | StringConst
// 变量声明 VarDecl → [ 'static' ] BType VarDef { ',' VarDef } ';' // i
// 变量定义 VarDef → Ident [ '[' ConstExp ']' ] | Ident [ '[' ConstExp ']' ] '=' InitVal // k
// 变量初值 InitVal → Exp | '{' [ Exp { ',' Exp } ] '}' | StringConst
// 函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block // j
// 主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block // j
// 函数类型 FuncType → 'void' | 'int' | 'char'
// 函数形参表 FuncFParams → FuncFParam { ',' FuncFParam }
// 函数形参 FuncFParam → BType Ident ['[' ']'] // k
// 语句块 Block → '{' { BlockItem } '}'
// 语句块项 BlockItem → Decl | Stmt
// 语句 Stmt → LVal '=' Exp ';' // i
// | [Exp] ';' // i
// | Block
// | 'if' '(' Cond ')' Stmt [ 'else' Stmt ] // j
// | 'while' '(' Cond ')' Stmt // j
// | 'switch' '(' Exp ')' '{' { CaseStmt } '}' // j
// | 'break' ';' | 'continue' ';' // i
// | 'return' [Exp] ';' // i
// | 'printf' '(' StringConst { ',' Exp } ')' ';' // e, l
// Case语句 CaseStmt → 'case' Number ':' { Stmt } | 'default' ':' { Stmt }
// 表达式 Exp → AddExp
// 条件表达式 Cond → LOrExp
// 左值表达式 LVal → Ident ['[' Exp ']'] // k
// 基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number // j
// 数值 Number → IntConst | CharConst
// 一元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp | '(' BType ')' UnaryExp // j
// 单目运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中
// 函数实参表 FuncRParams → Exp { ',' Exp }
// 乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
// 加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp
// 关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
// 相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
// 逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp
// 逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp
// 常量表达式 ConstExp → AddExp 注：使用的 Ident 必须是常量
#include "parser.h"

const char * ParTokenKindName(int kind){
    #define X(name,num) [num]= "<" #name ">", 
    static const char * ParTokenName[]={
        #include "../config/parser_token.def"
    };
    #undef X
    if(kind<0 || kind >= PTK_COUNT) return "<Error>";
    return ParTokenName[kind];
}
Parser::Parser(const vector<LexToken> & lexTokenList,ErrorController * errorController):
    lexTokenList(lexTokenList),
    errorController(errorController),
    curPos(0)
    {}

ParToken * Parser::scan_parser(){
    return parse_CompUnit();
}


ParToken * Parser::make_ParToken(ParTokenKind kind){
    parTokenPool.push_back(make_unique<struct ParToken>());
    ParToken * node = parTokenPool.back().get();
    node->type=kind;
    return node;
} 



const LexToken & Parser::get_cur(){
    return get_offset_next(0);
}
const LexToken & Parser::get_next(){
    return get_offset_next(1);
}
const LexToken & Parser::get_offset_next(int offset){
    static const LexToken eofToken{Eof,string(),0};   //这里是因为返回引用,所以需要一个静态变量
    if(curPos>=0&&curPos+offset<lexTokenList.size()){      //本身位置就不可以违法,所以>=0
        return lexTokenList[curPos+offset];
    }
    return eofToken;
}
void Parser::move_backward(){
    if(curPos>0) curPos--;
}
void Parser::move_forward(){
    if(curPos<lexTokenList.size()) curPos++;
}
void Parser::match(ParToken * father, LexTokenKind tar){
    if(get_cur().tokenType==tar){
        ParToken * node =make_ParToken(Leaf);
        node->lexToken=&get_cur();
        father->add(node);      
        move_forward();
    }else{
        errorController->register_ijk(tar,get_prev_line());
    }
}

bool Parser::check(LexTokenKind tar,int offset=0){
    if(tar==get_offset_next(offset).tokenType) return true;
    return false;
}
bool Parser::check_any(initializer_list<LexTokenKind> list,int offset=0){
    for(auto & kind :list){
        if(get_offset_next(offset).tokenType==kind) return true;
    }
    return false;
}
bool Parser::check_Btype(int offset=0){
    if(get_offset_next(offset).tokenType==INTTK||get_offset_next(offset).tokenType==CHARTK) return true;
    return false;
}
int Parser::get_prev_line(){
    if(curPos<=0) return 1;
    return lexTokenList[curPos-1].line_num;
}







// 编译单元 CompUnit → {Decl} {FuncDef} MainFuncDef
ParToken * Parser::parse_CompUnit(){
    ParToken * node = make_ParToken(CompUnit);
    while(is_Decl_ahead()){  
        node->add(parse_Decl());
    }
    while (is_FuncDef_ahead())
    {   
        node->add(parse_FuncDef());
    }
    node->add(parse_MainFuncDef());
    return node;
}
bool Parser::is_Decl_ahead(){    //所有is开头的判断均为    "可能是"!!!
    //严格来说,是当前场景下,可以靠is函数检查,而不会导致FIRST的混淆
    if(check_any({CONSTTK,STATICTK})||check_Btype()) 
        return !is_FuncDef_ahead()&&!is_MainFunDef_ahead();
    return false;
    //额,我想我只能这样判断,主要是判断这个太麻烦了
}
bool Parser::is_FuncDef_ahead(){
    if(check(VOIDTK)) return true;
    if(!check_Btype()) return false;  //额, int; 确实是合法的,我打算把它分类为Decl
    if(!check(IDENFR,1)) return false;
    if(!check(LPARENT,2)) return false;
    return true;
}
bool Parser::is_MainFunDef_ahead(){
    if(!check(INTTK)) return false;
    if(check(MAINTK,1)) return true;
    return false;
}

// 声明 Decl → ConstDecl | VarDecl
ParToken * Parser::parse_Decl(){
    ParToken * node = make_ParToken(Decl);
    if(check(CONSTTK)){
        node->add(parse_ConstDecl());
    }else {
        node->add(parse_VarDecl());
    }
    return node;
}
// 常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';' // i
ParToken * Parser::parse_ConstDecl(){
    ParToken * node = make_ParToken(ConstDecl);
    match(node,CONSTTK);
    node->add(parse_BType());
    node->add(parse_ConstDef());
    while (check(COMMA))
    {
        match(node,COMMA);
        node->add(parse_ConstDef());
    }
    match(node,SEMICN);
    return node;
}
// 基本类型 BType → 'int' | 'char'
ParToken * Parser::parse_BType(){
    ParToken * node = make_ParToken(BType);
    if(check(INTTK)){
        match(node,INTTK);
    }else{
        match(node,CHARTK);
    }
    return node;
}
// 常量定义 ConstDef → Ident [ '[' ConstExp ']' ] '=' ConstInitVal // k
ParToken * Parser::parse_ConstDef(){
    ParToken * node = make_ParToken(ConstDef);
    match(node,IDENFR);
    if(check(LBRACK)){
        match(node,LBRACK);
        node->add(parse_ConstExp());
        match(node,RBRACK);
    }
    match(node,ASSIGN);
    node->add(parse_ConstInitVal());
    return node;
}
// 常量初值 ConstInitVal → ConstExp | '{' [ ConstExp { ',' ConstExp } ] '}' | StringConst
ParToken * Parser::parse_ConstInitVal(){
    ParToken * node = make_ParToken(ConstInitVal);
    if(check(LBRACE)){
        match(node,LBRACE);
        if(is_any_Exp_ahead()){
            node->add(parse_ConstExp());
            while (check(COMMA)){
                match(node,COMMA);
                node->add(parse_ConstExp());
            }
        }
        match(node,RBRACE);
    }else if(check(STRCON)){
        match(node,STRCON);
    }else{
         node->add(parse_ConstExp());       
    }

    return node;
}
bool Parser::is_any_Exp_ahead(){   //所有的Exp,包括Cond和FuncRParams,除了Primary_Exp
    return check_any({IDENFR,LPARENT,INTCON,CHARCON,PLUS,MINU,NOT});
}
//至少写到现在还没有FISRT重叠


// 变量声明 VarDecl → [ 'static' ] BType VarDef { ',' VarDef } ';' // i
ParToken * Parser::parse_VarDecl(){
    ParToken * node = make_ParToken(VarDecl);
    if(check(STATICTK)) match(node,STATICTK);
    node->add(parse_BType());
    node->add(parse_VarDef());
    while (check(COMMA))
    {
        match(node,COMMA);
        node->add(parse_VarDef());
    }
    match(node,SEMICN);
    return node;
}
// 变量定义 VarDef → Ident [ '[' ConstExp ']' ] | Ident [ '[' ConstExp ']' ] '=' InitVal // k
ParToken * Parser::parse_VarDef(){
    ParToken * node = make_ParToken(VarDef);
    match(node,IDENFR);
    if(check(LBRACK)){
        match(node,LBRACK);
        node->add(parse_ConstExp());
        match(node,RBRACK);
    }
    if(check(ASSIGN)){
        match(node,ASSIGN);
        node->add(parse_InitVal());
    }
    return node;
}
// 变量初值 InitVal → Exp | '{' [ Exp { ',' Exp } ] '}' | StringConst
ParToken * Parser::parse_InitVal(){
    ParToken * node = make_ParToken(InitVal);
    if(check(LBRACE)){
        match(node,LBRACE);
        if(is_any_Exp_ahead()){
            node->add(parse_Exp());
            while (check(COMMA))
            {
                match(node,COMMA);
                node->add(parse_Exp());
            }
        }
        match(node,RBRACE);
    }else if(check(STRCON)){
        match(node,STRCON);
    }else {
        node->add(parse_Exp());
    }
    return node;
}

// 函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block // j
ParToken * Parser::parse_FuncDef(){
    ParToken * node = make_ParToken(FuncDef);
    node->add(parse_FuncType());
    match(node,IDENFR);
    match(node,LPARENT);
    if(is_FuncFParams_or_FuncFParam_ahead()){     //额,就是Btype裹了一层
        node->add(parse_FuncFParams());
    }
    match(node,RPARENT);
    node->add(parse_Block());
    return node;
}
bool Parser::is_FuncFParams_or_FuncFParam_ahead(){
    return check_Btype();
}
// 主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block // j
ParToken * Parser::parse_MainFuncDef(){
    ParToken * node = make_ParToken(MainFuncDef);
    match(node,INTTK);
    match(node,MAINTK);
    match(node,LPARENT);
    match(node,RPARENT);
    node->add(parse_Block());

    return node;
}
// 函数类型 FuncType → 'void' | 'int' | 'char'
ParToken * Parser::parse_FuncType(){
    ParToken * node = make_ParToken(FuncType);
    if(check(VOIDTK)) match(node,VOIDTK);
    else if(check(INTTK)) match(node,INTTK);
    else match(node,CHARTK);  
    return node;
}
// 函数形参表 FuncFParams → FuncFParam { ',' FuncFParam }
ParToken * Parser::parse_FuncFParams(){
    ParToken * node = make_ParToken(FuncFParams);
    node->add(parse_FuncFParam());
    while (check(COMMA))
    {
        match(node,COMMA);
        node->add(parse_FuncFParam());
    }
    return node;
}
// 函数形参 FuncFParam → BType Ident ['[' ']'] // k
ParToken * Parser::parse_FuncFParam(){
    ParToken * node = make_ParToken(FuncFParam);
    node->add(parse_BType());
    match(node,IDENFR);
    if(check(LBRACK)){
        match(node ,LBRACK);
        match(node ,RBRACK);
    }
    return node;
}
// 语句块 Block → '{' { BlockItem } '}'
ParToken * Parser::parse_Block(){
    ParToken * node = make_ParToken(Block);
    match(node,LBRACE);
    while(is_Stmt_ahead()||is_Decl_ahead()){    
        node->add(parse_BlockItem());
    }
    match(node,RBRACE);
    return node;
}
// 语句块项 BlockItem → Decl | Stmt
ParToken * Parser::parse_BlockItem(){
    ParToken * node = make_ParToken(BlockItem);
    if(is_Stmt_ahead()){
        node->add(parse_Stmt());
    }else {
        node->add(parse_Decl());
    }
    return node;
}

// 语句 Stmt → LVal '=' Exp ';' // i
// | [Exp] ';' // i
// | Block
// | 'if' '(' Cond ')' Stmt [ 'else' Stmt ] // j
// | 'while' '(' Cond ')' Stmt // j
// | 'switch' '(' Exp ')' '{' { CaseStmt } '}' // j
// | 'break' ';' | 'continue' ';' // i
// | 'return' [Exp] ';' // i
// | 'printf' '(' StringConst { ',' Exp } ')' ';' // e, l
ParToken * Parser::parse_Stmt(){
    ParToken * node = make_ParToken(Stmt);
    if(check(IDENFR)){

        node->add(parse_LVal());
        match(node,ASSIGN);
        node->add(parse_Exp());
        match(node,SEMICN);

    }else if(check(SEMICN)){

        match(node,SEMICN);

    }else if(is_any_Exp_ahead()){

        node->add(parse_Exp());
        match(node,SEMICN);

    }else if(check(LBRACE)){

        node->add(parse_Block());

    }else if(check(IFTK)){

        match(node,IFTK);
        match(node,LPARENT);
        node->add(parse_Cond());
        match(node,RPARENT);
        node->add(parse_Stmt());
        if(check(ELSETK)){
            match(node,ELSETK);
            node->add(parse_Stmt());
        }

    }else if(check(WHILETK)){
        match(node,WHILETK);
        match(node,LPARENT);
        node->add(parse_Cond());
        match(node,RPARENT);
        node->add(parse_Stmt());


    // | 'switch' '(' Exp ')' '{' { CaseStmt } '}' // j
    // | 'break' ';' | 'continue' ';' // i
    // | 'return' [Exp] ';' // i
    //好多啊,好多啊
    }else if(check(SWITCHTK)){

        match(node,SWITCHTK);
        match(node,LPARENT);
        node->add(parse_Exp());
        match(node,RPARENT);
        match(node,LBRACE);
        while (is_CaseStmt_ahead())
        {
            node->add(parse_CaseStmt());
        }
        match(node,RBRACE);

    }else if(check(BREAKTK)){

        match(node,BREAKTK);
        match(node,SEMICN);

    }else if(check(CONTINUETK)){   //为什么要合并啊啊啊啊啊

        match(node,CONTINUETK);
        match(node,SEMICN);

    }else if(check(RETURNTK)){

        match(node,RETURNTK);
        if(is_any_Exp_ahead()){
            node->add(parse_Exp());
        }
        match(node,SEMICN);

    }else if(check(PRINTFTK)){

        match(node,PRINTFTK);
        match(node,LPARENT);
        match(node,STRCON);
        while (check(COMMA))
        {
            match(node,COMMA);
            node->add(parse_Exp());
        }
        match(node,RPARENT);
        match(node,SEMICN);

    }
    return node;
}
//啊啊啊啊啊啊啊啊终于写完了


bool Parser::is_Stmt_ahead(){  //应该是没有重合
    return is_any_Exp_ahead()||check_any({IDENFR,SEMICN,LBRACE,IFTK,WHILETK,SWITCHTK,BREAKTK,CONTINUETK,RETURNTK,PRINTFTK});
}//按顺序的呀


bool Parser::is_CaseStmt_ahead(){  
    return check_any({CASETK,DEFAULTTK});
}

// Case语句 CaseStmt → 'case' Number ':' { Stmt } | 'default' ':' { Stmt }
ParToken * Parser::parse_CaseStmt(){
    ParToken * node = make_ParToken(CaseStmt);
    if(check(CASETK)){
        match(node,CASETK);
        node->add(parse_Number());
    }else if(check(DEFAULTTK)){
        match(node,DEFAULTTK);
    }
    match(node,COLON);
    while (is_Stmt_ahead())
    {
        printf("--------------------->\n");
        node->add(parse_Stmt());
    }
    return node;
}

//一刻也没有为Stmt哀悼,接下来赶到的是.....Exp!

// 表达式 Exp → AddExp
ParToken * Parser::parse_Exp(){
    ParToken * node = make_ParToken(Exp);
    node->add(parse_AddExp());
    return node;
}
// 条件表达式 Cond → LOrExp
ParToken * Parser::parse_Cond(){
    ParToken * node = make_ParToken(Cond);
    node->add(parse_LOrExp());
    return node;
}
// 左值表达式 LVal → Ident ['[' Exp ']'] // k
ParToken * Parser::parse_LVal(){
    ParToken * node = make_ParToken(LVal);
    match(node,IDENFR);
    if(check(LBRACK)){
        match(node,LBRACK);
        node->add(parse_Exp());
        match(node,RBRACK);
    }
    return node;
}
// 基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number // j
ParToken * Parser::parse_PrimaryExp(){
    ParToken * node = make_ParToken(PrimaryExp);
    if(check(LPARENT)){
        match(node,LPARENT);
        node->add(parse_Exp());
        match(node,RPARENT);
    }else if(check(IDENFR)){            //这里懒得新建is函数了,不管他了我累了嘻嘻
        node->add(parse_LVal());
    }else if(check(INTCON)||check(CHARCON)){
        node->add(parse_Number());
    }
    return node;
}
// 数值 Number → IntConst | CharConst
ParToken * Parser::parse_Number(){
    ParToken * node = make_ParToken(Number);
    if(check(INTCON)){
        match(node,INTCON); 
    }else if(check(CHARCON)){
        match(node,CHARCON);
    }
    return node;
}
// 一元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp | '(' BType ')' UnaryExp // j
ParToken * Parser::parse_UnaryExp(){
    ParToken * node = make_ParToken(UnaryExp);
    if(is_TypeCast_ahead()){
        match(node,LPARENT);
        node->add(parse_BType());
        match(node,RPARENT);
        node->add(parse_UnaryExp());
    }else if(is_PLUS_MINU_NOT_ahead()){
        node->add(parse_UnaryOp());
    }else if(is_FuncCall_ahead()){
        match(node,IDENFR);
        match(node,LPARENT);
        if(is_any_Exp_ahead()){   //饿啊,这个东西包括FRP
            node->add(parse_FuncRParams());
        }
        match(node,RPARENT);
    }else {
        node->add(parse_PrimaryExp());
    }
    return node;
}
bool Parser::is_PLUS_MINU_NOT_ahead(){
    return check_any({PLUS,MINU,NOT});
}   
bool Parser::is_FuncCall_ahead(){  //针对UnaryExp的第二分支
    return check(IDENFR,0)&&check(LPARENT,1);
}
bool Parser::is_TypeCast_ahead(){  //针对UnaryExp的第四分支
    return check(LPARENT,0)&&(check(INTTK,1)||check(CHARTK,1));
}
// 单目运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中
ParToken * Parser::parse_UnaryOp(){
    ParToken * node = make_ParToken(UnaryOp);
    if(check(PLUS)){
        match(node,PLUS);
    }else if(check(MINU)){
        match(node,MINU);
    }else if(check(NOT)){
        match(node,NOT);
    }
    return node;
}

// 函数实参表 FuncRParams → Exp { ',' Exp }
ParToken * Parser::parse_FuncRParams(){
    ParToken * node = make_ParToken(FuncRParams);
    node->add(parse_Exp());
    while (check(COMMA))
    {
        match(node,COMMA);
        node->add(parse_Exp());
    }
    return node;
}
// 乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
//改造一下: MulExp → UnaryExp { ('*' | '/' | '%') UnaryExp }
ParToken * Parser::parse_MulExp(){
    ParToken * node = make_ParToken(MulExp);
    node->add(parse_UnaryExp());
    while (is_MULT_DIV_MOD_ahead())
    {
        if(check(MULT)){
            match(node,MULT);
        }else if(check(DIV)){
            match(node,DIV);
        }else if(check(MOD)){
            match(node,MOD);
        }
        node->add(parse_UnaryExp());
    }
    
    return node;
}
bool Parser::is_MULT_DIV_MOD_ahead(){
    return check_any({MULT,DIV,MOD});
}
// 加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp
//AddExp → MulExp { ('+' | '−') MulExp}
ParToken * Parser::parse_AddExp(){
    ParToken * node = make_ParToken(AddExp);
    node->add(parse_MulExp());
    while (is_PLUS_MINU_ahead())
    {
        if(check(PLUS)){
            match(node,PLUS);
        }else if(check(MINU)){
            match(node,MINU);
        }
        node->add(parse_MulExp());
    }
    
    return node;
}

bool Parser::is_PLUS_MINU_ahead(){
    return check_any({PLUS,MINU});
}
// 关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
// RelExp → AddExp { ('<' | '>' | '<=' | '>=') AddExp}
ParToken * Parser::parse_RelExp(){
    ParToken * node = make_ParToken(RelExp);
    node->add(parse_AddExp());
    while (is_LSS_GRE_LEQ_GEQ_ahead())
    {
        if(check(LSS)){
            match(node,LSS);
        }else if(check(GRE)){
            match(node,GRE);
        }else if(check(LEQ)){
            match(node,LEQ);
        }else if(check(GEQ)){
            match(node,GEQ);
        }
        node->add(parse_AddExp());
    }
    
    return node;
}
bool Parser::is_LSS_GRE_LEQ_GEQ_ahead(){
    return check_any({LSS,GRE,LEQ,GEQ});
}
// 相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
//EqExp → RelExp { ('==' | '!=') RelExp}
ParToken * Parser::parse_EqExp(){
    ParToken * node = make_ParToken(EqExp);
    node->add(parse_RelExp());
    while (is_EQL_NEQ_ahead())
    {
        if(check(EQL)){
            match(node,EQL);
        }else if(check(NEQ)){
            match(node,NEQ);
        }
        node->add(parse_RelExp());
    }
    return node;
}
bool Parser::is_EQL_NEQ_ahead(){
    return check_any({EQL,NEQ});
}
// 逻辑与表达式 LAndExp → EqExp {'&&' EqExp}
ParToken * Parser::parse_LAndExp(){
    ParToken * node = make_ParToken(LAndExp);
    node->add(parse_EqExp());
    while (check(AND))
    {
        match(node,AND);
        node->add(parse_EqExp());
    }
    return node;
}
// 逻辑或表达式 LOrExp → LAndExp { '||' LAndExp}
ParToken * Parser::parse_LOrExp(){
    ParToken * node = make_ParToken(LOrExp);
    node->add(parse_LAndExp());
    while (check(OR))
    {
        match(node,OR);
        node->add(parse_LAndExp());
    }
    return node;
}
// 常量表达式 ConstExp → AddExp 注：使用的 Ident 必须是常量
ParToken * Parser::parse_ConstExp(){
    ParToken * node = make_ParToken(ConstExp);
    node->add(parse_AddExp());
    return node;
}
//终于写完了,真是我chovy