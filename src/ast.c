#include "ast.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern const char* keywords[];
extern const size_t KEYWORD_COUNT;

const char* declared_variables[100];
size_t declared_variables_count = 0;
const char* declared_functions[100];
size_t declared_functions_count = 0;

AST* ast_create() {
    AST* ast = calloc(1, sizeof(AST));
    ast->root = NULL;
    return ast;
}

void ast_destroy(AST* ast) {
    destroy_node(ast->root);
    free(ast);
}

void ast_print(AST* ast) {
    print_node(ast->root, 0);
}

void ast_build(AST* ast, Lexer* lexer) {
    lexer_lexall(lexer, false);
    lexer_set_cursor(lexer, 0);
    ast->root = ast_parse_program(lexer);
}

Node* ast_parse_program(Lexer* lexer) {
    Node* program = create_node(NODE_PROGRAM, NULL);
    Token* token = lexer_peek_token(lexer, 0);
    while (token->type != TOKEN_EOF) {
        Node* statement = ast_parse_statement(lexer);
        if (statement != NULL) {
            node_add_child(program, statement);
        }
        token = lexer_peek_token(lexer, 0);
    }
    return program;
}

Node* ast_parse_statement(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    Node* statement = NULL;
    if (token->type == TOKEN_KEYWORD) {
        if (strcmp(token->value, keywords[KEYWORD_FNC]) == 0) {
            statement = ast_parse_function(lexer);
        } else if (strcmp(token->value, keywords[KEYWORD_RET]) == 0) {
            statement = create_node(NODE_RETURN_STATEMENT, NULL);
            lexer_advance_cursor(lexer, 1);
            Node* expression = ast_parse_expression(lexer);
            node_add_child(statement, expression);
        }
    } else if (token->type == TOKEN_IDENTIFIER) {
        return ast_parse_assignment(lexer);
    } else if (token->type == TOKEN_PUNCTUATION) {
        if (strcmp(token->value, ";") == 0) {
            statement = create_node(NODE_NULL_LITERAL, NULL);
        }
    } else if (token->type == TOKEN_COMMENT) {
        statement = create_node(NODE_COMMENT, token->value);
    } else {
        printf("Unexpected token: %s\n", token->value);
        assert(false);
    }

    lexer_advance_cursor(lexer, 1);
    if (lexer_peek_token(lexer, 0)->type == TOKEN_PUNCTUATION && strcmp(lexer_peek_token(lexer, 0)->value, ";") == 0) {
        lexer_advance_cursor(lexer, 1);
    }

    return statement;
}

Node* ast_parse_function(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    assert(token->type == TOKEN_KEYWORD);
    assert(strcmp(token->value, keywords[KEYWORD_FNC]) == 0);
    Node* function = create_node(NODE_FUNCTION_DECLARATION, token->value);
    token = lexer_peek_token(lexer, 1);
    if (token->type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier after function keyword, got %s\n", token->value);
        assert(false);
    }

    Node* identifier = create_node(NODE_IDENTIFIER, token->value);
    node_add_child(function, identifier);
    token = lexer_peek_token(lexer, 2);
    assert(token->type == TOKEN_PUNCTUATION && strcmp(token->value, "(") == 0 && "Function arguments must be enclosed in parentheses");
    lexer_advance_cursor(lexer, 3);
    while (true) {
        Node* argument = ast_parse_function_argument(lexer);
        if (argument == NULL) {
            break;
        } else {
            node_add_child(function, argument);
        }
    }

    token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, ":") != 0) {
        fprintf(stderr, "Expected colon after function arguments, got %s\n", token->value);
        assert(false);
    }
    token = lexer_peek_token(lexer, 1);
    if (token->type != TOKEN_TYPEANNOTATION) {
        fprintf(stderr, "Expected type annotation after colon in function declaration, got %s\n", token->value);
        assert(false);
    }
    Node* type = create_node(NODE_TYPE, token->value);
    node_add_child(function, type);

    token = lexer_peek_token(lexer, 2);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ";") == 0) {
        lexer_advance_cursor(lexer, 1);
        return function;
    }

    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, "{") != 0) {
        fprintf(stderr, "Expected opening brace or \";\" after function declaration, got %s\n", token->value);
        assert(false);
    }
    lexer_advance_cursor(lexer, 2);
    Node* block = ast_parse_block(lexer);
    node_add_child(function, block);
    return function;
}

Node* ast_parse_function_argument(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ")") == 0) {
        lexer_advance_cursor(lexer, 1);
        return NULL;
    }
    token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier as function argument, got %s\n", token->value);
        assert(false);
    }
    Node* argument = create_node(NODE_FUNCTION_ARGUMENT, token->value);
    token = lexer_peek_token(lexer, 1);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ":") != 0) {
        fprintf(stderr, "Expected type declaration after function argument identifier, got %s\n", token->value);
        assert(false);
    }
    token = lexer_peek_token(lexer, 2);
    if (token->type != TOKEN_TYPEANNOTATION) {
        fprintf(stderr, "Expected type identifier after function argument type declaration, got %s\n", token->value);
        assert(false);
    }
    Node* type = create_node(NODE_TYPE, token->value);
    node_add_child(argument, type);
    token = lexer_peek_token(lexer, 3);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ",") == 0) {
        lexer_advance_cursor(lexer, 4);
    } else if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ")") == 0) {
        lexer_advance_cursor(lexer, 3);
    } else {
        fprintf(stderr, "Expected comma or closing parenthesis after function argument, got %s\n", token->value);
        assert(false);
    }
    return argument;
}

Node* ast_parse_block(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, "{") != 0) {
        fprintf(stderr, "Expected opening brace at beginning of block, got %s\n", token->value);
        assert(false);
    }

    Node* block = create_node(NODE_BLOCK_STATEMENT, NULL);
    lexer_advance_cursor(lexer, 1);
    while (true) {
        token = lexer_peek_token(lexer, 0);
        if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, "}") == 0) {
            break;
        }
        Node* statement = ast_parse_statement(lexer);
        if (statement == NULL) {
            break;
        } else {
            node_add_child(block, statement);
        }
    }
    // assert(lexer_peek_token(lexer, 1)->type == TOKEN_PUNCTUATION && strcmp(lexer_peek_token(lexer, 0)->value, "}") == 0);
    return block;
}

Node* ast_parse_assignment(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier as left-hand side of assignment, got %s\n", token->value);
        assert(false);
    }
    Node* identifier = create_node(NODE_IDENTIFIER, token->value);
    token = lexer_peek_token(lexer, 1);

    if (token->type == TOKEN_OPERATOR && strcmp(token->value, "=") == 0) {
        if (declared_variables_count == 0) {
            fprintf(stderr, "Cannot assign to undeclared variable %s\n", token->value);
            assert(false);
        }

        for (size_t i = 0; i < declared_variables_count; i++) {
            if (strcmp(declared_variables[i], identifier->data) == 0) {
                break;
            } else if (i == declared_variables_count - 1) {
                fprintf(stderr, "Cannot assign to undeclared variable %s\n", token->value);
                assert(false);
            }
        }

        lexer_advance_cursor(lexer, 2);
        Node* expression = ast_parse_expression(lexer);
        Node* assignment = create_node(NODE_ASSIGNMENT, NULL);
        node_add_child(assignment, identifier);
        node_add_child(assignment, expression);
        return assignment;
    }

    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, ":") != 0) {
        fprintf(stderr, "Expected colon after identifier in assignment, got %s\n", token->value);
        assert(false);
    }
    token = lexer_peek_token(lexer, 2);
    if (token->type != TOKEN_TYPEANNOTATION) {
        fprintf(stderr, "Expected type annotation after colon in assignment, got %s\n", token->value);
        assert(false);
    }
    Node* type = create_node(NODE_TYPE, token->value);
    node_add_child(identifier, type);
    token = lexer_peek_token(lexer, 3);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ";") == 0) {
        lexer_advance_cursor(lexer, 4);
        identifier->type = NODE_VARIABLE_DECLARATION;
        return identifier;
    } else if (token->type == TOKEN_OPERATOR && strcmp(token->value, "=") == 0) {
        lexer_advance_cursor(lexer, 4);
        Node* expression = ast_parse_expression(lexer);
        Node* assignment = create_node(NODE_ASSIGNMENT, NULL);
        node_add_child(assignment, identifier);
        node_add_child(assignment, expression);
        return assignment;
    } else {
        fprintf(stderr, "Expected semicolon or assignment operator after type annotation in assignment, got %s\n", token->value);
        assert(false);
    }

    return NULL;
}

Node* ast_parse_expression(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    Node* expression = create_node(NODE_EXPRESSION, NULL);
    while (true) {
        token = lexer_peek_token(lexer, 0);

        switch (token->type) {
            case TOKEN_IDENTIFIER:
                Token* next_tok = lexer_peek_token(lexer, 1);
                if (next_tok->type == TOKEN_PUNCTUATION && strcmp(next_tok->value, "(") == 0) {
                    // node_add_child(expression, ast_parse_call_expression(lexer));
                    //  TODO: Implement call expressions
                } else {
                    node_add_child(expression, create_node(NODE_IDENTIFIER, token->value));
                }
                break;
            case TOKEN_STRING:
                node_add_child(expression, create_node(NODE_STRING_LITERAL, token->value));
                break;
            case TOKEN_NUMBER:
                node_add_child(expression, create_node(NODE_NUMERIC_LITERAL, token->value));
                break;
            case TOKEN_FLOAT_NUM:
                node_add_child(expression, create_node(NODE_FLOAT_LITERAL, token->value));
                break;
            case TOKEN_KEYWORD:
                if (strcmp(token->value, keywords[KEYWORD_TRUE]) == 0) {
                    node_add_child(expression, create_node(NODE_TRUE_LITERAL, token->value));
                } else if (strcmp(token->value, keywords[KEYWORD_FALSE]) == 0) {
                    node_add_child(expression, create_node(NODE_FALSE_LITERAL, token->value));
                } else if (strcmp(token->value, keywords[KEYWORD_NULL]) == 0) {
                    node_add_child(expression, create_node(NODE_NULL_LITERAL, token->value));
                } else {
                    fprintf(stderr, "Unexpected keyword in expression: %s\n", token->value);
                    assert(false);
                }
                break;
            case TOKEN_OPERATOR:
                node_add_child(expression, create_node(NODE_OPERATOR, token->value));
                break;
            case TOKEN_PUNCTUATION:
                if (strcmp(token->value, "(") == 0) {
                    node_add_child(expression, ast_parse_expression(lexer));
                } else if (strcmp(token->value, ")") == 0) {
                    return expression;
                } else if (strcmp(token->value, ",") == 0) {
                    return expression;
                } else if (strcmp(token->value, ";") == 0) {
                    return expression;
                } else {
                    fprintf(stderr, "Unexpected punctuation in expression: %s\n", token->value);
                    assert(false);
                }
                break;
            case TOKEN_COMMENT:
                break;
            default:
                fprintf(stderr, "Unexpected token in expression: %s\n", token->value);
                assert(false);
        }
        lexer_advance_cursor(lexer, 1);
    }

    return expression;
}