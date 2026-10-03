#ifndef COMPILER_ERRORCONTROLLER_H
#define COMPILER_ERRORCONTROLLER_H
#include <vector> 
#include <cstdio>
#include "../config/enums.h"
#include "../config/structs.h"
#include <algorithm>
using namespace std;
// X(LEX_INVALID_TOKEN,   'a', "invalid token")
// X(SEM_IDENT_REDEF,     'b', "ident redefined")
// X(SEM_IDENT_UNDEF,     'c', "ident undefined")
// X(SEM_FN_PARAM_NUM,    'd', "function parameter number mismatch")
// X(SEM_TYPE_MISMATCH,   'e', "type mismatch")
// X(SEM_FN_RET_TYPE,     'f', "function should not return value")
// X(SEM_FN_RET_MISS,     'g', "function missing return value")
// X(SEM_CONST_ASSIGN,    'h', "constant assignment")
// X(SYN_MISS_SEMICN,     'i', "missing semicolon")
// X(SYN_MISS_PARENT,     'j', "missing parenthesis")
// X(SYN_MISS_BRACK,      'k', "missing bracket")
// X(SEM_PRINTF_MISMATCH, 'l', "printf format mismatch")
// X(SEM_BREAK_CONTINUE,  'm', "break or continue outside loop or switch")
// X(SYN_CASE_DUP,        'n', "duplicated case or default in switch")



class ErrorController {
public:
    void register_error(errorKind type, int lineNum);
    void report_error(FILE * err);
    void register_ijk(LexTokenKind tar,int lineNum);
    bool has_error();
private:
    vector <struct Error> errorList;
};






#endif