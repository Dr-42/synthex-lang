#pragma once

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_FLOAT_NUM,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_KEYWORD,
    TOKEN_VARIABLE_TYPE,
    TOKEN_PUNCTUATION,
    TOKEN_OPERATOR,
    TOKEN_COMMENT,
    TOKEN_TYPEANNOTATION,
    TOKEN_TOTAL,
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;