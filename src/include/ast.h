#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "lexer.h"
#include "node.h"
#include "token.h"
#include "utils/ast_data.h"

typedef struct {
    Node* root;
    ASTData* data;
} AST;

AST* ast_create();
void ast_destroy(AST* ast);

void ast_print(AST* ast);
void ast_print_declarations();
void _ast_error(int src_line, char* src_file, Token* token, char* message, ...);
#define ast_error(token, message, ...) _ast_error(__LINE__, __FILE__, token, message, ##__VA_ARGS__)

void ast_build(AST* ast, Lexer* lexer);
Node* ast_parse_program(Lexer* lexer);
Node* ast_parse_statement(Lexer* lexer);
Node* ast_parse_function(Lexer* lexer);
Node* ast_parse_if_statement(Lexer* lexer, bool is_elif);
Node* ast_parse_while_statement(Lexer* lexer);
Node* ast_parse_function_argument(Lexer* lexer);
Node* ast_parse_block(Lexer* lexer);
Node* ast_parse_variable_declaration(Lexer* lexer);
Node* ast_parse_array_declaration(Lexer* lexer);
Node* ast_parse_assignment(Lexer* lexer);
Node* ast_parse_array_assignment(Lexer* lexer);
Node* ast_parse_array_expression(Lexer* lexer, size_t array_dim);
Node* ast_parse_pointer_type(Lexer* lexer, size_t* ptr_depth);
Node* ast_parse_pointer_declaration(Lexer* lexer);
Node* ast_parse_pointer_deref(Lexer* lexer);
Node* ast_parse_expression(Lexer* lexer);
Node* ast_parse_array_index(Lexer* lexer);
Node* ast_parse_call_expression(Lexer* lexer);
Node* ast_parse_struct_declaration(Lexer* lexer);
Node* ast_parse_struct_access(Lexer* lexer);
Node* ast_parse_struct_member_assignment(Lexer* lexer);
