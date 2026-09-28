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
