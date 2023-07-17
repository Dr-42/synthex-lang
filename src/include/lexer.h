#pragma once

#include <stdio.h>

#include "token.h"

typedef struct {
    char *filename;
    char *contents;
    size_t line;
    size_t column;
    size_t index;
} Lexer;

Lexer *lexer_create(char *filename);
void lexer_destroy(Lexer *lexer);

Token *lexer_next_token(Lexer *lexer);
Token *lexer_peek_token(Lexer *lexer, size_t offset);
Token *lexer_create_token(Lexer *lexer, TokenType type, size_t start, size_t end);
void lexer_print_token(Token *token);
void lexer_print_tokens(Lexer *lexer);

char *token_type_to_string(TokenType type);