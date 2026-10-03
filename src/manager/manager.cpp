#include "manager.h"

CompilerManager::CompilerManager(FILE * in, FILE * out, FILE * err):
         errorController(),
         lexer(&errorController, lexTokenList),
         parser(lexTokenList,&errorController),
         in(in),
         out(out),
         err(err),
         parTreeRoot(nullptr)
{}
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
void CompilerManager::debug_report_lexer(){
    if(errorController.has_error()){
        report_error();
    }
    report_token_list();
}
void CompilerManager::debug_report_parser(){
    report_parser_tool(parTreeRoot);
}
void CompilerManager::report_parser_tool(ParToken * node){
    printf("%s %s\n",LexTokenKindName(node->lexToken->tokenType),node->lexToken->raw_string.c_str());
    if(node->type!=Leaf) {
        for (auto & child : node->childs)
        {
            report_parser_tool(child);   
        }
        printf("%s\n",ParTokenKindName(node->type));
    }

}
void CompilerManager::scan_parser(){
    parTreeRoot=parser.scan_parser();
}

