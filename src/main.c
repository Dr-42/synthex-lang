#include <stdio.h>
#include <string.h>
#include <signal.h>

#define DR42_TRACE_IMPLEMENTATION
#include "trace.h"

#include "tests.h"

#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "utils/ast_data.h"

void sigsegv_handler(int signum) {
    printf("Caught segfault %d\n", signum);
    print_trace();
    exit(1);
}


int main(int argc, char *argv[]) {
    signal(SIGSEGV, sigsegv_handler);
    if (argc != 4) {
        printf("Usage: %s <filename> -o <output>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "test") == 0) {
        test_all();
        return 0;
    }

    char* filename = argv[1];
    Lexer *lexer = lexer_create(filename);
    if (lexer == NULL) {
        printf("Failed to create lexer\n");
        return 1;
    }
    // lexer_print_tokens(lexer);

    AST *ast = ast_create();
    ast_build(ast, lexer);
    ast_print(ast);
    ast_data_print(ast->data);
    // ast_print_declarations();


    // Find the index where the argument is "-o"
    int index = -1;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        printf("Usage: %s <filename> -o <output>\n", argv[0]);
        return 1;
    }

    // Get the filename after "-o"
    char* ll_filename = argv[index+1];
    if (ll_filename == NULL) {
        printf("Usage: %s <filename> -o <output>\n", argv[0]);
        return 1;
    }

    ast_to_llvm(ast, lexer->filename, ll_filename, true);

    ast_destroy(ast);
    lexer_destroy(lexer);
    return 0;
}
