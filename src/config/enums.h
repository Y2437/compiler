#ifndef COMPILER_ENUMS_H
#define COMPILER_ENUMS_H
#include <map>
#include <string>
using namespace std;

#define X(name, num) name = num,
enum LexTokenKind {
#include "lexer_token.def"
LTK_COUNT
};
#undef X


#define X(name,ch,explain) name=(int) ch,
enum errorKind{
    #include "errors.def"
};
#undef X

#define X(name,num) name = num,
enum ParTokenKind{
    #include "parser_token.def"
    PTK_COUNT
};
#undef X





#endif