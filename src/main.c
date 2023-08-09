#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "utils/ast_data.h"

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
    ast_data_print(ast->data);
    // ast_print_declarations();

    ast_to_llvm(ast, lexer->filename);

    ast_destroy(ast);
    lexer_destroy(lexer);
    return 0;
}