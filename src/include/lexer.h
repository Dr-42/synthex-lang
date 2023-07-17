#pragma once

#include <stdio.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_KEYWORD,
    TOKEN_PUNCTUATION,
    TOKEN_OPERATOR,
    TOKEN_COMMENT,
    TOKEN_WHITESPACE,
    TOKEN_TYPEANNOTATION,
    TOKEN_TOTAL,
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

typedef struct {
    char *contents;
    char *filename;
    size_t line;
    size_t column;
    size_t index;
} Lexer;

Lexer *lexer_create(char *filename, char *contents);
void lexer_destroy(Lexer *lexer);

Token *lexer_next_token(Lexer *lexer);
Token *lexer_create_token(Lexer *lexer, TokenType type, size_t start, size_t end);
void lexer_print_token(Token *token);
void lexer_print_tokens(Lexer *lexer);

char *token_type_to_string(TokenType type);