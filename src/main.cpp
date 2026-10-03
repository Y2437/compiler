#include "manager/manager.h"
#define ONLINE_JUDGE 1

using namespace std;

int main() {      //优雅,永不过时
    FILE* in = stdin;
    FILE* err = stderr;
    FILE* out = stdout;
    if(ONLINE_JUDGE){
        in = fopen("testfile.txt", "r");
        err = fopen("error.txt", "w");
        out = fopen("lexer.txt", "w");
    }
    CompilerManager manager=CompilerManager(in,out,err);
    manager.scan_source();
    manager.scan_lexer();
    manager.debug_report_lexer();
    return 0;
}
