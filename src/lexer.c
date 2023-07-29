#include "lexer.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static Token tokens[1000];
static uint32_t token_count = 0;

const char *keywords[] = {
    "fnc",
    "inc",
    "true",
    "false",
    "if",
    "elif",
    "else",
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
    "stat"
    "as",
    "null",
};

const char *types[] = {
    "i8",
    "i16",
    "i32",
    "i64",
    "f32",
    "f64",
    "str",
    "chr",
    "bln",
    "void",
    "ptr",
};

const char *operators[] = {
    "...",
    "+=",
    "-=",
    "*=",
    "/=",
    "%=",
    "==",
    "!=",
    "<=",
    ">=",
    "&&",
    "||",
    "<<",
    ">>",
    "++",
    "--",
    ".",
    "+",
    "-",
    "*",
    "/",
    "%",
    "=",
    "<",
    ">",
    "!",
    "&",
    "|",
    "^",
    "~",
};

const char *punctuation[] = {
    "(",
    ")",
    "{",
    "}",
    "[",
    "]",
    ",",
    ";",
    ":",
};

const char *comments[] = {
    "//",
    "/*",
    "*/",
    "///",
};

const size_t KEYWORD_COUNT = (sizeof(keywords) / sizeof(keywords[0]));
const size_t TYPE_COUNT = (sizeof(types) / sizeof(types[0]));
const size_t OPERATOR_COUNT = (sizeof(operators) / sizeof(operators[0]));
const size_t PUNCTUATION_COUNT = (sizeof(punctuation) / sizeof(punctuation[0]));
const size_t COMMENT_COUNT = (sizeof(comments) / sizeof(comments[0]));

Lexer *lexer_create(char *filename) {
    Lexer *lexer = calloc(1, sizeof(Lexer));
    lexer->filename = filename;
    lexer->line = 1;
    lexer->column = 1;
    lexer->index = 0;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    lexer->contents = calloc(size + 1, sizeof(char));
    fread(lexer->contents, sizeof(char), size, file);
    fclose(file);

    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    for (uint32_t i = 0; i < token_count; i++) {
        free(tokens[i].value);
    }
    free(lexer);
}

Token *lexer_peek_token(Lexer *lexer, size_t offset) {
    uint32_t index = lexer->index;
    uint32_t line = lexer->line;
    uint32_t column = lexer->column;
    Token *token = &tokens[lexer->index + offset];
    lexer->index = index;
    lexer->line = line;
    lexer->column = column;
    return token;
}

void lexer_set_cursor(Lexer *lexer, size_t index) {
    lexer->index = index;
}

void lexer_advance_cursor(Lexer *lexer, int32_t offset) {
    lexer->index += offset;
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
    while ((token = lexer_next_token(lexer))->type != TOKEN_EOF) {
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
        case TOKEN_FLOAT_NUM:
            return "TOKEN_FLOAT_NUM";
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
        case TOKEN_TYPEANNOTATION:
            return "TOKEN_TYPEANNOTATION";
        default:
            return "Unknown token type";
    }
}

Token *lexer_next_token(Lexer *lexer) {
    char c;
    while ((c = lexer->contents[lexer->index]) != '\0') {
        // Skip whitespaces
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            lexer->index++;
            continue;
        }

        // Match keywords
        for (uint32_t i = 0; i < KEYWORD_COUNT; i++) {
            if (strncmp(&lexer->contents[lexer->index], keywords[i], strlen(keywords[i])) == 0) {
                lexer->index += strlen(keywords[i]);
                return lexer_create_token(lexer, TOKEN_KEYWORD, lexer->index - strlen(keywords[i]), lexer->index);
            }
        }

        // Match types
        for (uint32_t i = 0; i < TYPE_COUNT; i++) {
            if (strncmp(&lexer->contents[lexer->index], types[i], strlen(types[i])) == 0) {
                lexer->index += strlen(types[i]);
                return lexer_create_token(lexer, TOKEN_TYPEANNOTATION, lexer->index - strlen(types[i]), lexer->index);
            }
        }

        if (isalpha(c) || c == '_') {
            size_t start = lexer->index;
            while (isalpha(lexer->contents[lexer->index]) || isdigit(lexer->contents[lexer->index]) || lexer->contents[lexer->index] == '_' || lexer->contents[lexer->index] == '-') {
                lexer->index++;
            }
            return lexer_create_token(lexer, TOKEN_IDENTIFIER, start, lexer->index);
        }

        if (isdigit(c)) {
            size_t start = lexer->index;
            TokenType type = TOKEN_NUMBER;
            while (isdigit(lexer->contents[lexer->index]) || lexer->contents[lexer->index] == '.') {
                lexer->index++;
                if (lexer->contents[lexer->index] == '.') {
                    type = TOKEN_FLOAT_NUM;
                    lexer->index++;
                }
            }
            return lexer_create_token(lexer, type, start, lexer->index);
        }

        if (c == '\"') {
            size_t start = lexer->index++;
            while (lexer->contents[lexer->index] != '\"') {
                lexer->index++;
            }
            lexer->index++;
            return lexer_create_token(lexer, TOKEN_STRING, start, lexer->index);
        }

        // Match punctuation
        for (uint32_t i = 0; i < PUNCTUATION_COUNT; i++) {
            if (c == punctuation[i][0]) {
                lexer->index++;
                return lexer_create_token(lexer, TOKEN_PUNCTUATION, lexer->index - 1, lexer->index);
            }
        }

        // Match operators
        for (uint32_t i = 0; i < OPERATOR_COUNT; i++) {
            if (strncmp(&lexer->contents[lexer->index], operators[i], strlen(operators[i])) == 0) {
                lexer->index += strlen(operators[i]);
                return lexer_create_token(lexer, TOKEN_OPERATOR, lexer->index - strlen(operators[i]), lexer->index);
            }
        }

        // Match comments (simplified: only single-line comments are considered)
        if (c == '/' && lexer->contents[lexer->index + 1] == '/') {
            size_t start = lexer->index;
            while (lexer->contents[lexer->index] != '\n') {
                lexer->index++;
            }
            return lexer_create_token(lexer, TOKEN_COMMENT, start, lexer->index);
        }

        // Match comments (simplified: only multi-line comments are considered)
        if (c == '/' && lexer->contents[lexer->index + 1] == '*') {
            size_t start = lexer->index;
            while (lexer->contents[lexer->index] != '*' || lexer->contents[lexer->index + 1] != '/') {
                lexer->index++;
            }
            lexer->index += 2;
            return lexer_create_token(lexer, TOKEN_COMMENT, start, lexer->index);
        }

        lexer->index++;
    }

    return lexer_create_token(lexer, TOKEN_EOF, lexer->index, lexer->index);
}

void lexer_lexall(Lexer *lexer, bool print) {
    Token *token = NULL;
    while ((token = lexer_next_token(lexer))->type != TOKEN_EOF) {
        if (print)
            lexer_print_token(token);
    }
}
