#ifndef COMPILER_MANAGER_H
#define COMPILER_MANAGER_H

#include "../error/errorController.h"
#include "../lexer/lexer.h"

using namespace std;
class CompilerManager{
    private:
        vector<Token> tokenList;
        ErrorController  errorController;
        Lexer  lexer;
        FILE * in;
        FILE * out;
        FILE * err;
    public:
        CompilerManager(FILE * in,FILE * out ,FILE * err);
        void report_error();
        void report_token_list();
        void scan_source();
        void debug_report();
        void scan_lexer();
};

#endif