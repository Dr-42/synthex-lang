#include "lexer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static Token tokens[1000];
static uint32_t token_count = 0;

const char *keywords[] = {
    "fnc",
    "i8",
    "i16",
    "i32",
    "i64",
    "u8",
    "u16",
    "u32",
    "u64",
    "f32",
    "f64",
    "str",
    "chr",
    "bln",
    "void",
    "true",
    "false",
    "if",
    "else",
    "elif",
    "while",
    "for",
    "in",
    "ret",
    "brk",
    "cont",
    "struct",
    "enum",
    "union",
    "pub",
    "prv",
    "const",
    "sizeof",
};

#define KEYWORD_COUNT (sizeof(keywords) / sizeof(keywords[0]))

Lexer *lexer_create(char *filename, char *contents) {
    Lexer *lexer = calloc(1, sizeof(Lexer));
    lexer->filename = filename;
    lexer->contents = contents;
    lexer->line = 1;
    lexer->column = 1;
    lexer->index = 0;
    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    for (uint32_t i = 0; i < token_count; i++) {
        free(tokens[i].value);
    }
    free(lexer);
}

Token *lexer_next_token(Lexer *lexer) {
    if (lexer->contents[lexer->index] == '\0') {
        return NULL;
    }

    char c = lexer->contents[lexer->index];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        lexer->index++;
        return lexer_next_token(lexer);
    }

    if (c == '/' && lexer->contents[lexer->index + 1] == '/') {
        lexer->index += 2;
        return lexer_next_token(lexer);
    }

    if (c == '/' && lexer->contents[lexer->index + 1] == '*') {
        lexer->index += 2;
        while (lexer->contents[lexer->index] != '\0') {
            if (lexer->contents[lexer->index] == '*' && lexer->contents[lexer->index + 1] == '/') {
                lexer->index += 2;
                break;
            }
            lexer->index++;
        }
        return lexer_next_token(lexer);
    }

    if (c == '\"') {
        lexer->index++;
        uint32_t start = lexer->index;
        while (lexer->contents[lexer->index] != '\"') {
            lexer->index++;
        }
        uint32_t end = lexer->index;
        lexer->index++;
        return lexer_create_token(lexer, TOKEN_STRING, start, end);
    }

    if (c == '\'') {
        lexer->index++;
        uint32_t start = lexer->index;
        while (lexer->contents[lexer->index] != '\'') {
            lexer->index++;
        }
        uint32_t end = lexer->index;
        lexer->index++;
        return lexer_create_token(lexer, TOKEN_CHAR, start, end);
    }

    if (c == '#') {
        lexer->index++;
        uint32_t start = lexer->index;
        while (lexer->contents[lexer->index] != '\n') {
            lexer->index++;
        }
        uint32_t end = lexer->index;
        lexer->index++;
        return lexer_create_token(lexer, TOKEN_COMMENT, start, end);
    }

    if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        uint32_t start = lexer->index;
        while (lexer->contents[lexer->index] == '_' || (lexer->contents[lexer->index] >= 'a' && lexer->contents[lexer->index] <= 'z') || (lexer->contents[lexer->index] >= 'A' && lexer->contents[lexer->index] <= 'Z') || (lexer->contents[lexer->index] >= '0' && lexer->contents[lexer->index] <= '9')) {
            lexer->index++;
        }
        uint32_t end = lexer->index;
        char *value = calloc(end - start + 1, sizeof(char));
        memcpy(value, lexer->contents + start, end - start);
        for (uint32_t i = 0; i < KEYWORD_COUNT; i++) {
            if (strcmp(value, keywords[i]) == 0) {
                return lexer_create_token(lexer, TOKEN_KEYWORD, start, end);
            }
        }
        return lexer_create_token(lexer, TOKEN_IDENTIFIER, start, end);
    }

    if (c >= '0' && c <= '9') {
        uint32_t start = lexer->index;
        while (lexer->contents[lexer->index] >= '0' && lexer->contents[lexer->index] <= '9') {
            lexer->index++;
        }
        uint32_t end = lexer->index;
        return lexer_create_token(lexer, TOKEN_NUMBER, start, end);
    }

    if (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == ';' || c == ',' || c == '.') {
        lexer->index++;
        return lexer_create_token(lexer, TOKEN_PUNCTUATION, lexer->index - 1, lexer->index);
    }

    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|') {
        lexer->index++;
        return lexer_create_token(lexer, TOKEN_OPERATOR, lexer->index - 1, lexer->index);
    }

    if (c == ':') {
        lexer->index++;
        return lexer_create_token(lexer, TOKEN_TYPEANNOTATION, lexer->index - 1, lexer->index);
    }

    return NULL;
}

Token *lexer_create_token(Lexer *lexer, TokenType type, size_t start, size_t end) {
    Token *token = &tokens[token_count++];
    token->type = type;
    token->value = calloc(end - start + 1, sizeof(char));
    memcpy(token->value, lexer->contents + start, end - start);
    return token;
}

void lexer_print_token(Token *token) {
    printf("Token: %s Value: %s\n", token_type_to_string(token->type), token->value);
}

void lexer_print_tokens(Lexer *lexer) {
    Token *token = NULL;
    while ((token = lexer_next_token(lexer)) != NULL) {
        lexer_print_token(token);
    }
}

char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF:
            return "TOKEN_EOF";
        case TOKEN_IDENTIFIER:
            return "TOKEN_IDENTIFIER";
        case TOKEN_NUMBER:
            return "TOKEN_NUMBER";
        case TOKEN_STRING:
            return "TOKEN_STRING";
        case TOKEN_CHAR:
            return "TOKEN_CHAR";
        case TOKEN_KEYWORD:
            return "TOKEN_KEYWORD";
        case TOKEN_PUNCTUATION:
            return "TOKEN_PUNCTUATION";
        case TOKEN_OPERATOR:
            return "TOKEN_OPERATOR";
        case TOKEN_COMMENT:
            return "TOKEN_COMMENT";
        case TOKEN_WHITESPACE:
            return "TOKEN_WHITESPACE";
        case TOKEN_TYPEANNOTATION:
            return "TOKEN_TYPEANNOTATION";
        default:
            return "Unknown token type";
    }
}