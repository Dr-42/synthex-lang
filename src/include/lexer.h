#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "token.h"

typedef struct {
    char *filename;
    char *contents;
    size_t line;
    size_t column;
    size_t index;
    Token *tokens;
    size_t token_count;
} Lexer;

typedef enum {
    KEYWORD_FNC = 0,
    KEYWORD_INC,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    KEYWORD_IF,
    KEYWORD_ELIF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_FOR,
    KEYWORD_IN,
    KEYWORD_RET,
    KEYWORD_BRK,
    KEYWORD_CONT,
    KEYWORD_STRUCT,
    KEYWORD_ENUM,
    KEYWORD_UNION,
    KEYWORD_PUB,
    KEYWORD_PRV,
    KEYWORD_CONST,
    KEYWORD_STAT,
    KEYWORD_AS,
    KEYWORD_NULL,
    KEYWORD_TOTAL,
} KeywordType;

typedef enum {
    DATA_TYPE_I8 = 0,
    DATA_TYPE_I16,
    DATA_TYPE_I32,
    DATA_TYPE_I64,
    DATA_TYPE_F32,
    DATA_TYPE_F64,
    DATA_TYPE_STR,
    DATA_TYPE_CHR,
    DATA_TYPE_BLN,
    DATA_TYPE_VOID,
    DATA_TYPE_PTR,
    DATA_TYPE_TOTAL,
} DataType;

Lexer *lexer_create(char *filename);
void lexer_destroy(Lexer *lexer);

Token *lexer_next_token(Lexer *lexer);
Token *lexer_peek_token(Lexer *lexer, size_t offset);
void lexer_set_cursor(Lexer *lexer, size_t index);
void lexer_advance_cursor(Lexer *lexer, int32_t offset);

Token *lexer_create_token(Lexer *lexer, TokenType type, size_t start, size_t end);
void lexer_lexall(Lexer *lexer, bool print);
void lexer_print_token(Token *token);
void lexer_print_tokens(Lexer *lexer);

char *token_type_to_string(TokenType type);
DataType get_data_type(const char *type_str);
KeywordType get_keyword_type(const char *keyword_str);