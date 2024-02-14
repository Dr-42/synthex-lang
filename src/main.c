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
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "test") == 0) {
        test_all();
        return 0;
    }

    Lexer *lexer = lexer_create(argv[1]);
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

    char* filename = argv[1];
    char ll_filename[256];
    strcpy(ll_filename, filename);
    strcat(ll_filename, ".ll");

    ast_to_llvm(ast, lexer->filename, ll_filename, true);

    ast_destroy(ast);
    lexer_destroy(lexer);
    return 0;
}
