#include "lexer.h"
#include "token.h"

#include "utils/ast_data.h"

#include "trace.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define array_length(array) (sizeof(array) / sizeof(array[0]))

char** graveyard = NULL;
size_t graveyard_count = 0;

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
    "stat",
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
    "`",
};

const char *comments[] = {
    "//",
    "/*",
    "*/",
    "///",
};

char user_defined_types[256][256] = {0};
size_t user_defined_types_count = 0;
bool in_user_defined_type = false;

const size_t KEYWORD_COUNT = array_length(keywords);
const size_t BUILTIN_TYPE_COUNT = array_length(types);
const size_t OPERATOR_COUNT = array_length(operators);
const size_t PUNCTUATION_COUNT = array_length(punctuation);
const size_t COMMENT_COUNT = array_length(comments);

Lexer *lexer_create(char *filename) {
    Lexer *lexer = malloc(sizeof(Lexer));
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

    lexer->contents = malloc((size + 1) * sizeof(char));
    fread(lexer->contents, sizeof(char), size, file);
    fclose(file);

    lexer->tokens = NULL;
    lexer->token_count = 0;

    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    free(lexer->contents);
    free(lexer->tokens);
    free(lexer);

    for (size_t i = 0; i < graveyard_count; i++) {
        free(graveyard[i]);
    }
    graveyard_count = 0;
    graveyard = NULL;
}

Token *lexer_peek_token(Lexer *lexer, size_t offset) {
    Token *token = &(lexer->tokens[lexer->index + offset]);
    return token;
}

void lexer_set_cursor(Lexer *lexer, size_t index) {
    lexer->index = index;
}

void lexer_advance_cursor(Lexer *lexer, int32_t offset) {
    lexer->index += offset;
}

Token *lexer_create_token(Lexer *lexer, TokenType type, size_t start, size_t end) {
    if (lexer->token_count == 0) {
        lexer->tokens = calloc(1, sizeof(Token));
    } else {
        lexer->tokens = realloc(lexer->tokens, (lexer->token_count + 1) * sizeof(Token));
    }
    Token *token = &lexer->tokens[lexer->token_count++];
    token->type = type;
    token->value = calloc(end - start + 1, sizeof(char));
    memcpy(token->value, lexer->contents + start, end - start);
    token->value[end - start] = '\0';
    token->line = lexer->line;
    token->column = lexer->column;
    token->filename = lexer->filename;
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
        case TOKEN_DOC_COMMENT:
            return "TOKEN_DOC_COMMENT";
        case TOKEN_TYPEANNOTATION:
            return "TOKEN_TYPEANNOTATION";
        case TOKEN_TYPEDECLARATION:
            return "TOKEN_TYPEDECLARATION";
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
            if (c == '\n') {
                lexer->line++;
                lexer->column = 1;
            } else if (c == '\t') {
                lexer->column += 4;
            } else {
                lexer->column++;
            }
            continue;
        }

        if (isalpha(c) || c == '_') {
            size_t start = lexer->index;
            while (isalpha(lexer->contents[lexer->index]) || isdigit(lexer->contents[lexer->index]) || lexer->contents[lexer->index] == '_' || lexer->contents[lexer->index] == '-') {
                lexer->index++;
                lexer->column++;
            }
            if (in_user_defined_type) {
                snprintf(user_defined_types[user_defined_types_count], lexer->index - start + 1, "%s", &lexer->contents[start]);
                user_defined_types_count++;

                in_user_defined_type = false;
                Token* tok = lexer_create_token(lexer, TOKEN_TYPEDECLARATION, start, lexer->index);
                return tok;
            }

            // Match user defined types
            Token* tok = lexer_create_token(lexer, TOKEN_IDENTIFIER, start, lexer->index);

            for (size_t i = 0; i < user_defined_types_count; i++) {
                if (strcmp(tok->value, user_defined_types[i]) == 0) {
                    tok->type = TOKEN_TYPEANNOTATION;
                    break;
                }
            }

            // Match keywords
            for (uint32_t i = 0; i < KEYWORD_COUNT; i++) {
                if (strcmp(tok->value, keywords[i]) == 0) {
                    tok->type = TOKEN_KEYWORD;
                    if (strcmp(tok->value, "struct") == 0 || strcmp(tok->value, "enum") == 0 || strcmp(tok->value, "union") == 0) {
                        in_user_defined_type = true;
                    }
                    break;
                }
            }

            // Match types
            for (uint32_t i = 0; i < BUILTIN_TYPE_COUNT; i++) {
                if (strcmp(tok->value, types[i]) == 0) {
                    tok->type = TOKEN_TYPEANNOTATION;
                    break;
                }
            }
            return tok;
        }

        if (isdigit(c)) {
            size_t start = lexer->index;
            TokenType type = TOKEN_NUMBER;
            while (isdigit(lexer->contents[lexer->index]) || lexer->contents[lexer->index] == '.') {
                lexer->index++;
                if (lexer->contents[lexer->index] == '.') {
                    type = TOKEN_FLOAT_NUM;
                    lexer->index++;
                    lexer->column++;
                }
            }
            return lexer_create_token(lexer, type, start, lexer->index);
        }

        if (c == '\"') {
            size_t start = lexer->index++;
            while (lexer->contents[lexer->index] != '\"') {
                lexer->index++;
                lexer->column++;
            }
            lexer->index++;
            lexer->column++;
            return lexer_create_token(lexer, TOKEN_STRING, start, lexer->index);
        }

        // Match punctuation
        for (uint32_t i = 0; i < PUNCTUATION_COUNT; i++) {
            if (c == punctuation[i][0]) {
                lexer->index++;
                lexer->column++;
                return lexer_create_token(lexer, TOKEN_PUNCTUATION, lexer->index - 1, lexer->index);
            }
        }

        if (c == '/' && lexer->contents[lexer->index + 1] == '/' && lexer->contents[lexer->index + 2] == '/') {
            size_t start = lexer->index + 3;
            while (lexer->contents[lexer->index] != '\n') {
                lexer->index++;
                lexer->column++;
            }
            return lexer_create_token(lexer, TOKEN_DOC_COMMENT, start, lexer->index);
        }

        if (c == '/' && lexer->contents[lexer->index + 1] == '/') {
            size_t start = lexer->index + 2;
            while (lexer->contents[lexer->index] != '\n') {
                lexer->index++;
                lexer->column++;
            }
            return lexer_create_token(lexer, TOKEN_COMMENT, start, lexer->index);
        }

        if (c == '/' && lexer->contents[lexer->index + 1] == '*') {
            size_t start = lexer->index + 2;
            while (lexer->contents[lexer->index] != '*' || lexer->contents[lexer->index + 1] != '/') {
                lexer->index++;
                lexer->column++;
            }
            lexer->index += 2;
            lexer->column += 2;
            Token *token = lexer_create_token(lexer, TOKEN_COMMENT, start, lexer->index - 2);
            // Replace all newlines with spaces
            for (size_t i = 0; i < strlen(token->value); i++) {
                if (token->value[i] == '\n') {
                    token->value[i] = ' ';
                }
            }
            // Replace all tabs with spaces
            for (size_t i = 0; i < strlen(token->value); i++) {
                if (token->value[i] == '\t') {
                    token->value[i] = ' ';
                }
            }
            // Remove all spaces at the beginning of the comment
            while (token->value[0] == ' ') {
                token->value++;
            }
            // Remove all spaces at the end of the comment
            while (token->value[strlen(token->value) - 1] == ' ') {
                token->value[strlen(token->value) - 1] = '\0';
            }
            // Remove all more than 1 space in a row
            for (size_t i = 0; i < strlen(token->value); i++) {
                while (token->value[i] == ' ' && token->value[i + 1] == ' ') {
                    memmove(&token->value[i], &token->value[i + 1], strlen(token->value) - i);
                }
            }
            return token;
        }

        // Match operators
        for (uint32_t i = 0; i < OPERATOR_COUNT; i++) {
            if (strncmp(&lexer->contents[lexer->index], operators[i], strlen(operators[i])) == 0) {
                lexer->index += strlen(operators[i]);
                lexer->column += strlen(operators[i]);
                return lexer_create_token(lexer, TOKEN_OPERATOR, lexer->index - strlen(operators[i]), lexer->index);
            }
        }

        lexer->index++;
        lexer->column++;
    }

    return lexer_create_token(lexer, TOKEN_EOF, lexer->index, lexer->index);
}

void lexer_lexall(Lexer *lexer, bool print) {
    Token *token = NULL;
    bool encountered_equal = false;
    while ((token = lexer_next_token(lexer))->type != TOKEN_EOF) {
        if (print) {
            lexer_print_token(token);
        }
        // Fix ">>" operator
        if (token->type == TOKEN_OPERATOR && strcmp(token->value, "=") == 0) {
            encountered_equal = true;
        }
        if (token->type == TOKEN_PUNCTUATION) {
            if (strcmp(token->value, ";") == 0 || strcmp(token->value, "{") == 0 || strcmp(token->value, "}") == 0) {
                encountered_equal = false;
            }
        }
        if (!encountered_equal) {
            if (token->type == TOKEN_OPERATOR && strcmp(token->value, ">>") == 0) {
                token->type = TOKEN_OPERATOR;
                token->value = ">";
                // Add another token ">"
                Token *token2 = lexer_create_token(lexer, TOKEN_OPERATOR, lexer->index, lexer->index);
                token2->type = TOKEN_OPERATOR;
                token2->value = ">";
            }
        }
    }
}

DataType* get_data_type(const char *type_str, ASTData *data) {
    for (size_t i = 0; i < data->data_type_count; i++) {
        if (strcmp(type_str, data->data_types[i].name) == 0) {
            return &data->data_types[i];
        }
    }
    print_trace();
    fprintf(stderr, "Error: Type %s not found\n", type_str);
    exit(1);
}

KeywordType get_keyword_type(const char *keyword_str) {
    for (int i = 0; i < KEYWORD_TOTAL; i++) {
        if (strcmp(keyword_str, keywords[i]) == 0) {
            return i;
        }
    }

    return KEYWORD_TOTAL;
}

void lexer_graveyard(char *token) {
    graveyard = realloc(graveyard, (graveyard_count + 1) * sizeof(char*));
    graveyard[graveyard_count++] = token;
}
