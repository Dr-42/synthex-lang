#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
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

    ast_destroy(ast);
    lexer_destroy(lexer);
    return 0;
}