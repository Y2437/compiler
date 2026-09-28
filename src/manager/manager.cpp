#include "manager.h"

CompilerManager::CompilerManager(FILE * in,FILE * out ,FILE * err):
    in(in),
    out(out),
    err(err),
    errorController(ErrorController()),
    lexer(Lexer(&errorController,tokenList)){}
void CompilerManager::report_error(){
    errorController.report_error(err);
}
void CompilerManager::report_token_list(){
    lexer.report_token_list(out);
}
void CompilerManager::scan_source(){
    lexer.read_source(in);
}
void CompilerManager::scan_lexer(){
    lexer.scan();
}
void CompilerManager::debug_report(){
    if(errorController.has_error()){
        report_error();
    }
    report_token_list();
}

