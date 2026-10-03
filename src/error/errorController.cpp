#include "errorController.h"


void ErrorController::register_error(errorKind type,int lineNum ){
    errorList.push_back({type,lineNum});
}
void ErrorController::report_error(FILE * err){
    for(auto &  e : errorList){
        fprintf(err,"%d %c\n",e.lineNum,(char)e.type);
    }
}
bool ErrorController::has_error(){
    return !errorList.empty();
}
void ErrorController::register_ijk(LexTokenKind tar,int lineNum){
    LexTokenKind kind = tar;
    switch (kind)
    {
    case SEMICN:
        register_error(SYN_MISS_SEMICN,lineNum);
        break;
    case RPARENT:
        register_error(SYN_MISS_PARENT,lineNum);
        break;
    case RBRACK:
        register_error(SYN_MISS_BRACK,lineNum);
        break;
    default:
        break;
    }

}