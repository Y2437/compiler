#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H
using namespace std;
#include "../error/errorController.h"
#include "../config/enums.h"
#include "../config/structs.h"
#include <initializer_list>
#include <memory>
#include <vector>


const char * ParTokenKindName(int);


class Parser{
    public:
        Parser(const vector<LexToken> & lexTokenList,ErrorController * errorController);
        ParToken * scan_parser();

    private:
        const vector<LexToken> & lexTokenList;
        ErrorController * errorController;
        int curPos;
        vector<unique_ptr<ParToken> > parTokenPool;



        ParToken * make_ParToken(ParTokenKind);

        const LexToken & get_cur();
        const LexToken & get_next();
        const LexToken & get_offset_next(int offset);


        void move_forward();
        void move_backward();




        void match(ParToken * father, LexTokenKind);
        bool check(LexTokenKind,int offset);
        bool check_any(initializer_list<LexTokenKind>,int offset);
        bool check_Btype(int offset);
        int get_prev_line();


        bool is_Decl_ahead();
        bool is_FuncDef_ahead();
        bool is_MainFunDef_ahead();
        bool is_any_Exp_ahead();
        bool is_FuncFParams_or_FuncFParam_ahead();
        bool is_Stmt_ahead();
        bool is_CaseStmt_ahead();
        
        bool is_TypeCast_ahead();
        bool is_FuncCall_ahead();

        bool is_MULT_DIV_MOD_ahead();
        bool is_PLUS_MINU_ahead();
        bool is_LSS_GRE_LEQ_GEQ_ahead();
        bool is_EQL_NEQ_ahead();
        bool is_PLUS_MINU_NOT_ahead();
        bool is_Block_ahead();


        ParToken * parse_CompUnit();
        ParToken * parse_Decl();
        ParToken * parse_ConstDecl();
        ParToken * parse_BType();
        ParToken * parse_ConstDef();
        ParToken * parse_ConstInitVal();
        ParToken * parse_VarDecl();
        ParToken * parse_VarDef();
        ParToken * parse_InitVal();
        ParToken * parse_FuncDef();
        ParToken * parse_MainFuncDef();
        ParToken * parse_FuncType();
        ParToken * parse_FuncFParams();
        ParToken * parse_FuncFParam();
        ParToken * parse_Block();
        ParToken * parse_BlockItem();
        ParToken * parse_Stmt();
        ParToken * parse_CaseStmt();
        ParToken * parse_Exp();
        ParToken * parse_Cond();
        ParToken * parse_LVal();
        ParToken * parse_PrimaryExp();
        ParToken * parse_Number();
        ParToken * parse_UnaryExp();
        ParToken * parse_UnaryOp();
        ParToken * parse_FuncRParams();
        ParToken * parse_MulExp();
        ParToken * parse_AddExp();
        ParToken * parse_RelExp();
        ParToken * parse_EqExp();
        ParToken * parse_LAndExp();
        ParToken * parse_LOrExp();
        ParToken * parse_ConstExp();

};


#endif

