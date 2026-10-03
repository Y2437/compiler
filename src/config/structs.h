#ifndef COMPILER_STRUCTS_H
#define COMPILER_STRUCTS_H

using namespace std;
#include "enums.h"
#include <vector>
#include <cstdio>
struct Error {
    errorKind type;
    int lineNum;
};

struct LexToken {
    LexTokenKind tokenType;
    string raw_string;
    int line_num;
};

struct ParToken{
    ParTokenKind type;
    const LexToken * lexToken;
    vector <struct ParToken * > childs;

    ParToken * add(ParToken * token){
        this->childs.push_back(token);
        return this;
    }
    
};



#endif
