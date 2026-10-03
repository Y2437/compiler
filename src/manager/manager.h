#ifndef COMPILER_MANAGER_H
#define COMPILER_MANAGER_H

#include "../error/errorController.h"
#include "../lexer/lexer.h"
#include "../config/enums.h"
#include "../config/structs.h"
#include "../parser/parser.h"
using namespace std;
class CompilerManager{
    private:
        vector<LexToken> lexTokenList;
        ErrorController  errorController;
        Lexer  lexer;
        Parser parser;
        FILE * in;
        FILE * out;
        FILE * err;
        ParToken * parTreeRoot;
    public:
        CompilerManager(FILE * in,FILE * out ,FILE * err);
        void report_error();
        void report_token_list();
        void scan_source();
        void debug_report_lexer();
        void scan_lexer();
        void scan_parser();
        void debug_report_parser();
};

#endif