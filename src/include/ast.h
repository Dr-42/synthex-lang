#pragma once

#include <stdlib.h>

#include "lexer.h"
#include "node.h"
#include "token.h"

typedef struct {
    Node* root;
} AST;

AST* ast_create();
void ast_destroy(AST* ast);

void ast_print(AST* ast);
void ast_print_declarations();

void ast_build(AST* ast, Lexer* lexer);
Node* ast_parse_program(Lexer* lexer);
Node* ast_parse_statement(Lexer* lexer);
Node* ast_parse_function(Lexer* lexer);
Node* ast_parse_function_argument(Lexer* lexer);
Node* ast_parse_block(Lexer* lexer);
Node* ast_parse_assignment(Lexer* lexer);
Node* ast_parse_expression(Lexer* lexer);
Node* ast_parse_call_expression(Lexer* lexer);