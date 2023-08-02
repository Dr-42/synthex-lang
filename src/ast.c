#include "ast.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern const char* keywords[];
extern const size_t KEYWORD_COUNT;

extern Token tokens[];

typedef struct Function {
    const char* name;
    const char* return_type;
    const char* arguments[100];
    size_t argument_count;
} Function;

typedef struct Variable {
    const char* name;
    const char* type;
} Variable;

Variable declared_variables[100];
size_t declared_variables_count = 0;
Variable declared_arrays[100];
size_t declared_arrays_count = 0;
size_t declared_array_dims[100];
Function declared_functions[100];
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
    bool* indents = malloc(100 * sizeof(bool));  // Assume max depth of 100
    print_node(ast->root, indents, 0);
    free(indents);
}

void ast_print_declarations() {
    printf("Variables:\n");
    for (size_t i = 0; i < declared_variables_count; i++) {
        printf("\t%s: %s\n", declared_variables[i].name, declared_variables[i].type);
    }
    printf("Functions:\n");
    for (size_t i = 0; i < declared_functions_count; i++) {
        printf("\t%s: %s(", declared_functions[i].name, declared_functions[i].return_type);
        for (size_t j = 0; j < declared_functions[i].argument_count; j++) {
            printf("%s : %s", declared_functions[i].arguments[j], declared_variables[j].type);
            if (j != declared_functions[i].argument_count - 1) {
                printf(", ");
            }
        }
        printf(")\n");
    }
}

void _ast_error(int src_line, char* file, Token* token, char* message, ...) {
    va_list args;
    va_start(args, message);
    fprintf(stderr, "%s:%d %s:%zu:%zu: ", file, src_line, token->filename, token->line, token->column);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
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
        } else if (strcmp(token->value, keywords[KEYWORD_IF]) == 0) {
            statement = ast_parse_if_statement(lexer, false);
        } else if (strcmp(token->value, keywords[KEYWORD_WHILE]) == 0) {
            statement = ast_parse_while_statement(lexer);
        } else if (strcmp(token->value, keywords[KEYWORD_RET]) == 0) {
            statement = create_node(NODE_RETURN_STATEMENT, NULL);
            lexer_advance_cursor(lexer, 1);
            Node* expression = ast_parse_expression(lexer);
            node_add_child(statement, expression);
        } else if (strcmp(token->value, keywords[KEYWORD_BRK]) == 0) {
            statement = create_node(NODE_BRK_STATEMENT, NULL);
            lexer_advance_cursor(lexer, 1);
        } else if (strcmp(token->value, keywords[KEYWORD_CONT]) == 0) {
            statement = create_node(NODE_CONT_STATEMENT, NULL);
            lexer_advance_cursor(lexer, 1);
        } else {
            ast_error(token, "Unexpected keyword", token->value);
        }
    } else if (token->type == TOKEN_IDENTIFIER) {
        Token* next_token = lexer_peek_token(lexer, 1);
        if (next_token->type == TOKEN_PUNCTUATION && strcmp(next_token->value, "(") == 0) {
            statement = ast_parse_call_expression(lexer);
        } else if (next_token->type == TOKEN_PUNCTUATION && strcmp(next_token->value, ":") == 0) {
            next_token = lexer_peek_token(lexer, 2);
            if (next_token->type == TOKEN_TYPEANNOTATION) {
                statement = ast_parse_variable_declaration(lexer);
            } else if (next_token->type == TOKEN_PUNCTUATION && strcmp(next_token->value, "[") == 0) {
                statement = ast_parse_array_declaration(lexer);
            } else {
                ast_error(next_token, "Expected type annotation after colon in variable declaration, got %s\n", next_token->value);
            }
        } else if (next_token->type == TOKEN_PUNCTUATION && strcmp(next_token->value, "[") == 0) {
            statement = ast_parse_array_assignment(lexer);
        } else if (next_token->type == TOKEN_OPERATOR && strcmp(next_token->value, "=") == 0) {
            for (size_t i = 0; i < declared_variables_count; i++) {
                if (strcmp(declared_variables[i].name, token->value) == 0) {
                    return ast_parse_assignment(lexer);
                }
            }
            for (size_t i = 0; i < declared_arrays_count; i++) {
                if (strcmp(declared_arrays[i].name, token->value) == 0) {
                    return ast_parse_array_assignment(lexer);
                }
            }

            ast_error(token, "Cannot assign to undeclared variable %s\n", token->value);
        }
    } else if (token->type == TOKEN_PUNCTUATION) {
        if (strcmp(token->value, ";") == 0) {
            statement = create_node(NODE_NULL_LITERAL, NULL);
        }
    } else if (token->type == TOKEN_COMMENT) {
        statement = create_node(NODE_COMMENT, token->value);
    } else {
        ast_error(token, "Unexpected token: %s\n", token->value);
    }

    if (lexer_peek_token(lexer, 0)->type == TOKEN_PUNCTUATION && strcmp(lexer_peek_token(lexer, 0)->value, ";") == 0) {
        lexer_advance_cursor(lexer, 1);
    }

    return statement;
}

Node* ast_parse_function(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    assert(token->type == TOKEN_KEYWORD);
    assert(strcmp(token->value, keywords[KEYWORD_FNC]) == 0);
    Node* function = create_node(NODE_FUNCTION_DECLARATION, NULL);
    token = lexer_peek_token(lexer, 1);
    if (token->type != TOKEN_IDENTIFIER) {
        ast_error(token, "Expected identifier after function keyword, got %s\n", token->value);
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
        ast_error(token, "Expected colon after function arguments, got %s\n", token->value);
    }
    token = lexer_peek_token(lexer, 1);
    if (token->type != TOKEN_TYPEANNOTATION) {
        ast_error(token, "Expected type annotation after colon in function declaration, got %s\n", token->value);
    }
    Node* type = create_node(NODE_TYPE, token->value);
    node_add_child(function, type);

    token = lexer_peek_token(lexer, 2);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ";") == 0) {
        lexer_advance_cursor(lexer, 3);
        declared_functions[declared_functions_count].name = identifier->data;
        declared_functions[declared_functions_count].return_type = type->data;
        for (size_t i = 0; i < function->num_children; i++) {
            if (function->children[i]->type == NODE_FUNCTION_ARGUMENT) {
                declared_functions[declared_functions_count].arguments[declared_functions[declared_functions_count].argument_count] = function->children[i]->data;
                declared_functions[declared_functions_count].argument_count++;
            }
        }
        declared_functions_count++;
        return function;
    }

    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, "{") != 0) {
        ast_error(token, "Expected opening brace or \";\" after function declaration, got %s\n", token->value);
    }
    lexer_advance_cursor(lexer, 2);
    Node* block = ast_parse_block(lexer);
    node_add_child(function, block);

    declared_functions[declared_functions_count].name = identifier->data;
    declared_functions[declared_functions_count].return_type = type->data;
    for (size_t i = 0; i < function->num_children; i++) {
        if (function->children[i]->type == NODE_FUNCTION_ARGUMENT) {
            declared_functions[declared_functions_count].arguments[declared_functions[declared_functions_count].argument_count] = function->children[i]->data;
            declared_functions[declared_functions_count].argument_count++;
        }
    }

    declared_functions_count++;

    lexer_advance_cursor(lexer, 1);
    return function;
}

Node* ast_parse_function_argument(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    bool is_ellipsis = false;
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ")") == 0) {
        lexer_advance_cursor(lexer, 1);
        return NULL;
    }
    token = lexer_peek_token(lexer, 0);
    if (token->type == TOKEN_OPERATOR && strcmp(token->value, "...") == 0) {
        is_ellipsis = true;
        Token* next_token = lexer_peek_token(lexer, 1);
        if (next_token->type != TOKEN_PUNCTUATION || strcmp(next_token->value, ")") != 0) {
            ast_error(next_token, "Expected closing parenthesis after ellipsis in function argument, got %s\n", next_token->value);
        }
    } else if (token->type != TOKEN_IDENTIFIER) {
        ast_error(token, "Expected identifier as function argument, got %s\n", token->value);
    }
    Node* argument = create_node(NODE_FUNCTION_ARGUMENT, token->value);
    if (is_ellipsis) {
        lexer_advance_cursor(lexer, 1);
        return argument;
    }
    token = lexer_peek_token(lexer, 1);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ":") != 0 && !is_ellipsis) {
        ast_error(token, "Expected type declaration after function argument identifier, got %s\n", token->value);
    }
    token = lexer_peek_token(lexer, 2);
    if (token->type != TOKEN_TYPEANNOTATION) {
        ast_error(token, "Expected type identifier after function argument type declaration, got %s\n", token->value);
    }
    Node* type = create_node(NODE_TYPE, token->value);
    node_add_child(argument, type);
    token = lexer_peek_token(lexer, 3);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ",") == 0) {
        lexer_advance_cursor(lexer, 4);
    } else if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ")") == 0) {
        lexer_advance_cursor(lexer, 3);
    } else {
        ast_error(token, "Expected comma or closing parenthesis after function argument, got %s\n", token->value);
    }
    return argument;
}

Node* ast_parse_if_statement(Lexer* lexer, bool is_elif) {
    Token* token = lexer_peek_token(lexer, 0);
    assert(token->type == TOKEN_KEYWORD);
    if (is_elif) {
        assert(strcmp(token->value, keywords[KEYWORD_ELIF]) == 0);
    } else {
        assert(strcmp(token->value, keywords[KEYWORD_IF]) == 0);
    }
    Node* if_statement;
    if (is_elif) {
        if_statement = create_node(NODE_ELIF_STATEMENT, NULL);
    } else {
        if_statement = create_node(NODE_IF_STATEMENT, NULL);
    }
    lexer_advance_cursor(lexer, 1);
    Node* expression = ast_parse_expression(lexer);
    if (strcmp(lexer_peek_token(lexer, 0)->value, ")") == 0) {
        lexer_advance_cursor(lexer, 1);
    }
    node_add_child(if_statement, expression);
    token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, "{") != 0) {
        ast_error(token, "Expected opening brace after if statement expression, got %s\n", token->value);
    }

    Node* block = ast_parse_block(lexer);
    node_add_child(if_statement, block);
    lexer_advance_cursor(lexer, 1);

    token = lexer_peek_token(lexer, 0);

    if (token->type == TOKEN_KEYWORD && !is_elif && strcmp(token->value, keywords[KEYWORD_ELIF]) == 0) {
        while (true) {
            Node* elif_statement = ast_parse_if_statement(lexer, true);
            node_add_child(if_statement, elif_statement);
            token = lexer_peek_token(lexer, 0);
            if (token->type != TOKEN_KEYWORD || strcmp(token->value, keywords[KEYWORD_ELIF]) != 0) {
                break;
            }
        }
    }

    if (token->type == TOKEN_KEYWORD && !is_elif && strcmp(token->value, keywords[KEYWORD_ELSE]) == 0) {
        lexer_advance_cursor(lexer, 1);
        token = lexer_peek_token(lexer, 0);
        if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, "{") != 0) {
            ast_error(token, "Expected opening brace after else keyword, got %s\n", token->value);
        }
        Node* else_block = ast_parse_block(lexer);
        Node* else_statement = create_node(NODE_ELSE_STATEMENT, NULL);
        node_add_child(if_statement, else_statement);
        node_add_child(else_statement, else_block);
        lexer_advance_cursor(lexer, 1);
    }

    return if_statement;
}

Node* ast_parse_while_statement(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    assert(token->type == TOKEN_KEYWORD);
    assert(strcmp(token->value, keywords[KEYWORD_WHILE]) == 0);
    Node* while_statement = create_node(NODE_WHILE_STATEMENT, NULL);
    lexer_advance_cursor(lexer, 1);
    Node* expression = ast_parse_expression(lexer);
    if (strcmp(lexer_peek_token(lexer, 0)->value, ")") == 0) {
        lexer_advance_cursor(lexer, 1);
    }
    node_add_child(while_statement, expression);
    token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, "{") != 0) {
        ast_error(token, "Expected opening brace after if statement expression, got %s\n", token->value);
    }

    Node* block = ast_parse_block(lexer);
    node_add_child(while_statement, block);
    lexer_advance_cursor(lexer, 1);

    return while_statement;
}

Node* ast_parse_block(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, "{") != 0) {
        ast_error(token, "Expected opening brace at beginning of block, got %s\n", token->value);
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
    return block;
}

Node* ast_parse_variable_declaration(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_IDENTIFIER) {
        ast_error(token, "Expected identifier as left-hand side of variable declaration, got %s\n", token->value);
    }

    Node* identifier = create_node(NODE_IDENTIFIER, token->value);
    token = lexer_peek_token(lexer, 1);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ":") == 0) {
        token = lexer_peek_token(lexer, 2);
        if (token->type != TOKEN_TYPEANNOTATION) {
            ast_error(token, "Expected type annotation after colon in variable declaration, got %s\n", token->value);
        }
        Node* type = create_node(NODE_TYPE, token->value);
        token = lexer_peek_token(lexer, 3);

        declared_variables[declared_variables_count].name = identifier->data;
        declared_variables[declared_variables_count].type = type->data;
        declared_variables_count++;
        identifier->type = NODE_VARIABLE_DECLARATION;
        node_add_child(identifier, type);

        if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ";") == 0) {
            lexer_advance_cursor(lexer, 4);
            return identifier;
        } else if (token->type == TOKEN_OPERATOR && strcmp(token->value, "=") == 0) {
            lexer_advance_cursor(lexer, 2);
            tokens[lexer->index].type = TOKEN_IDENTIFIER;
            tokens[lexer->index].value = identifier->data;
            return identifier;
        } else {
            ast_error(token, "Expected semicolon or assignment operator after type annotation in variable declaration, got %s\n", token->value);
        }
    }
    return NULL;
}

Node* ast_parse_array_declaration(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_IDENTIFIER) {
        ast_error(token, "Expected identifier as left-hand side of array declaration, got %s\n", token->value);
    }

    Node* identifier = create_node(NODE_IDENTIFIER, token->value);
    token = lexer_peek_token(lexer, 3);
    if (token->type != TOKEN_TYPEANNOTATION) {
        ast_error(token, "Expected type annotation after colon in array declaration, got %s\n", token->value);
    }
    Node* type = create_node(NODE_TYPE, token->value);

    size_t idx = 4;
    size_t array_dims_count = 0;
    Node** array_dims = calloc(100, sizeof(Node*));
    while (true) {
        token = lexer_peek_token(lexer, idx);
        if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, "]") == 0) {
            idx++;
            break;
        }

        token = lexer_peek_token(lexer, idx);
        if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ";") == 0) {
            idx++;
        }

        token = lexer_peek_token(lexer, idx);
        if (token->type != TOKEN_NUMBER) {
            ast_error(token, "Expected number as array size, got %s\n", token->value);
        }
        Node* array_dim = create_node(NODE_NUMERIC_LITERAL, token->value);
        array_dims[array_dims_count] = array_dim;
        array_dims_count++;
        idx++;
    }

    Node* array_declaration = create_node(NODE_ARRAY_DECLARATION, NULL);
    node_add_child(array_declaration, identifier);
    node_add_child(array_declaration, type);
    for (size_t i = 0; i < array_dims_count; i++) {
        node_add_child(type, array_dims[i]);
    }

    free(array_dims);

    declared_arrays[declared_arrays_count].name = identifier->data;
    declared_arrays[declared_arrays_count].type = type->data;
    declared_array_dims[declared_arrays_count] = array_dims_count;
    declared_arrays_count++;

    token = lexer_peek_token(lexer, idx);

    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ";") == 0) {
        lexer_advance_cursor(lexer, idx + 1);
        return array_declaration;
    } else if (token->type == TOKEN_OPERATOR && strcmp(token->value, "=") == 0) {
        lexer_advance_cursor(lexer, idx - 1);
        tokens[lexer->index].type = TOKEN_IDENTIFIER;
        tokens[lexer->index].value = identifier->data;
        return array_declaration;
    } else {
        ast_error(token, "Expected semicolon or assignment operator after array declaration, got %s\n", token->value);
    }
    return NULL;
}

Node* ast_parse_assignment(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_IDENTIFIER) {
        ast_error(token, "Expected identifier as left-hand side of assignment, got %s\n", token->value);
    }
    Node* identifier = create_node(NODE_IDENTIFIER, token->value);
    token = lexer_peek_token(lexer, 1);

    if (token->type == TOKEN_OPERATOR && strcmp(token->value, "=") == 0) {
        if (declared_variables_count == 0) {
            ast_error(token, "Cannot assign to undeclared variable %s\n", (char*)identifier->data);
        }

        for (size_t i = 0; i < declared_variables_count; i++) {
            if (strcmp(declared_variables[i].name, identifier->data) == 0) {
                break;
            } else if (i == declared_variables_count - 1) {
                ast_error(token, "Cannot assign to undeclared variable %s\n", (char*)identifier->data);
            }
        }

        lexer_advance_cursor(lexer, 2);
        Node* expression = ast_parse_expression(lexer);
        Node* assignment = create_node(NODE_ASSIGNMENT, NULL);
        node_add_child(assignment, identifier);
        node_add_child(assignment, expression);
        return assignment;
    } else {
        ast_error(token, "Expected assignment operator after identifier in assignment, got %s\n", token->value);
    }
    return NULL;
}

Node* ast_parse_array_expression(Lexer* lexer, size_t array_dim) {
    Node* array_expression = create_node(NODE_ARRAY_EXPRESSION, NULL);
    if (array_dim == 1) {
        Token* token = lexer_peek_token(lexer, 0);
        if (token->type != TOKEN_PUNCTUATION && strcmp(token->value, "[") != 0) {
            ast_error(token, "Expected opening bracket in array expression, got %s\n", token->value);
        }
        lexer_advance_cursor(lexer, 1);
        while (true) {
            Token* token = lexer_peek_token(lexer, 0);
            if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, "]") == 0) {
                lexer_advance_cursor(lexer, 1);
                break;
            }
            Node* expression = ast_parse_expression(lexer);
            node_add_child(array_expression, expression);
        }
    } else {
        Token* token = lexer_peek_token(lexer, 0);
        if (token->type != TOKEN_PUNCTUATION && strcmp(token->value, "[") != 0) {
            ast_error(token, "Expected opening bracket in array expression, got %s\n", token->value);
        }
        lexer_advance_cursor(lexer, 1);
        while (true) {
            Node* array_expression_child = ast_parse_array_expression(lexer, array_dim - 1);
            node_add_child(array_expression, array_expression_child);
            Token* token = lexer_peek_token(lexer, 0);
            if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ",") == 0) {
                lexer_advance_cursor(lexer, 1);
                continue;
            } else if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, "]") == 0) {
                lexer_advance_cursor(lexer, 1);
                break;
            } else {
                ast_error(token, "Expected comma or closing bracket in array expression, got %s\n", token->value);
            }
        }
    }
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ";") == 0) {
        lexer_advance_cursor(lexer, 1);
    }
    return array_expression;
}

Node* ast_parse_array_assignment(Lexer* lexer) {
    // Two types of array assignment:
    // 1. Assigning to a single element
    // 2. Assigning the entire array
    Token* token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_IDENTIFIER) {
        ast_error(token, "Expected identifier as left-hand side of array assignment, got %s\n", token->value);
    }

    size_t array_idx = 0;
    for (size_t i = 0; i < declared_arrays_count; i++) {
        if (strcmp(declared_arrays[i].name, token->value) == 0) {
            array_idx = i;
            break;
        } else if (i == declared_arrays_count - 1) {
            ast_error(token, "Cannot assign to undeclared array %s\n", token->value);
        }
    }

    size_t array_dim = declared_array_dims[array_idx];

    Node* identifier = create_node(NODE_IDENTIFIER, token->value);
    token = lexer_peek_token(lexer, 1);
    if (token->type == TOKEN_OPERATOR && strcmp(token->value, "=") == 0) {
        // Assigning the entire array
        lexer_advance_cursor(lexer, 2);
        Node* expression = ast_parse_array_expression(lexer, array_dim);
        Node* assignment = create_node(NODE_ARRAY_ASSIGNMENT, NULL);
        node_add_child(assignment, identifier);
        node_add_child(assignment, expression);
        return assignment;
    } else if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, "[") == 0) {
        // Assigning to a single element
        // example syntax: arr[0][3][4] = 1;
        lexer_advance_cursor(lexer, 1);
        size_t dim_depth = array_dim;
        for (size_t i = 0; i < array_dim; i++) {
            Node* array_index = ast_parse_array_index(lexer);
            node_add_child(identifier, array_index);
            dim_depth--;
            Token* ntok = lexer_peek_token(lexer, 0);
            if (ntok->type == TOKEN_OPERATOR && strcmp(ntok->value, "=") == 0) {
                break;
            }
        }

        token = lexer_peek_token(lexer, 0);

        if (token->type != TOKEN_OPERATOR && strcmp(token->value, "=") != 0) {
            ast_error(token, "Expected = after array index of array length %d, got %s\n", array_dim, token->value);
        }

        lexer_advance_cursor(lexer, 1);
        Node* expression = ast_parse_expression(lexer);
        Node* assignment = create_node(NODE_ARRAY_ASSIGNMENT, NULL);
        node_add_child(assignment, identifier);
        node_add_child(assignment, expression);
        return assignment;
    } else {
        ast_error(token, "Expected assignment operator or opening bracket after identifier in array assignment, got %s\n", token->value);
    }
    return NULL;
}

Node* ast_parse_expression(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    Node* expression = create_node(NODE_EXPRESSION, NULL);
    size_t paren_count = 0;
    while (true) {
        token = lexer_peek_token(lexer, 0);

        switch (token->type) {
            case TOKEN_IDENTIFIER: {
                Token* next_tok = lexer_peek_token(lexer, 1);
                if (next_tok->type == TOKEN_PUNCTUATION && strcmp(next_tok->value, "(") == 0) {
                    node_add_child(expression, ast_parse_call_expression(lexer));
                } else if (next_tok->type == TOKEN_PUNCTUATION && strcmp(next_tok->value, "[") == 0) {
                    lexer_advance_cursor(lexer, 1);
                    Node* array_element = create_node(NODE_ARRAY_ELEMENT, token->value);
                    while (true) {
                        Node* array_index = ast_parse_array_index(lexer);
                        node_add_child(array_element, array_index);
                        Token* tok = lexer_peek_token(lexer, 0);
                        if (tok->type != TOKEN_PUNCTUATION || strcmp(tok->value, "[") != 0) {
                            lexer_advance_cursor(lexer, -1);
                            break;
                        }
                    }
                    node_add_child(expression, array_element);
                    break;
                } else {
                    node_add_child(expression, create_node(NODE_IDENTIFIER, token->value));
                }
                break;
            }
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
                    ast_error(token, "Unexpected keyword in expression: %s\n", token->value);
                }
                break;
            case TOKEN_OPERATOR:
                node_add_child(expression, create_node(NODE_OPERATOR, token->value));
                break;
            case TOKEN_PUNCTUATION:
                if (strcmp(token->value, "(") == 0) {
                    lexer_advance_cursor(lexer, 1);
                    paren_count++;
                    Node* paren_expression = ast_parse_expression(lexer);
                    node_add_child(expression, paren_expression);
                    continue;
                } else if (strcmp(token->value, ")") == 0) {
                    if (paren_count == 0) {
                        return expression;
                    } else {
                        paren_count--;
                        if (paren_count == 0) {
                            lexer_advance_cursor(lexer, 1);
                            return expression;
                        }
                    }
                } else if (strcmp(token->value, ",") == 0) {
                    lexer_advance_cursor(lexer, 1);
                    return expression;
                } else if (strcmp(token->value, ";") == 0) {
                    lexer_advance_cursor(lexer, 1);
                    return expression;
                } else if (strcmp(token->value, "]") == 0) {
                    return expression;
                } else {
                    ast_error(token, "Unexpected punctuation in expression: %s\n", token->value);
                }
                break;
            case TOKEN_COMMENT:
                break;
            default: {
                ast_error(token, "Unexpected token in expression: %s\n", token->value);
            }
        }
        lexer_advance_cursor(lexer, 1);
    }

    return expression;
}

Node* ast_parse_array_index(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);
    Node* array_index = create_node(NODE_EXPRESSION, NULL);
    if (token->type != TOKEN_PUNCTUATION && strcmp(token->value, "[") != 0) {
        ast_error(token, "Expected opening bracket in array index, got %s\n", token->value);
    }
    lexer_advance_cursor(lexer, 1);
    while (true) {
        token = lexer_peek_token(lexer, 0);
        if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, "]") == 0) {
            lexer_advance_cursor(lexer, 1);
            break;
        } else if (token->type == TOKEN_IDENTIFIER) {
            Node* identifier = create_node(NODE_IDENTIFIER, token->value);
            node_add_child(array_index, identifier);
        } else if (token->type == TOKEN_NUMBER) {
            Node* numeric_literal = create_node(NODE_NUMERIC_LITERAL, token->value);
            node_add_child(array_index, numeric_literal);
        } else if (token->type == TOKEN_OPERATOR) {
            Node* operator= create_node(NODE_OPERATOR, token->value);
            node_add_child(array_index, operator);
        } else if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, "[") == 0) {
            Node* array_index_child = ast_parse_array_index(lexer);
            node_add_child(array_index, array_index_child);
        } else {
            ast_error(token, "Unexpected token in array index: %s\n", token->value);
        }
        lexer_advance_cursor(lexer, 1);
    }

    return array_index;
}

Node* ast_parse_call_expression(Lexer* lexer) {
    Token* token = lexer_peek_token(lexer, 0);

    if (token->type != TOKEN_IDENTIFIER) {
        ast_error(token, "Expected identifier as left-hand side of call expression, got %s\n", token->value);
    }

    for (size_t i = 0; i < declared_functions_count; i++) {
        if (strcmp(declared_functions[i].name, token->value) == 0) {
            break;
        } else if (i == declared_functions_count - 1) {
            ast_error(token, "Cannot call undeclared function %s\n", token->value);
        }
    }

    Node* call_expression = create_node(NODE_CALL_EXPRESSION, NULL);
    Node* identifier = create_node(NODE_IDENTIFIER, token->value);
    node_add_child(call_expression, identifier);
    token = lexer_peek_token(lexer, 1);
    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, "(") != 0) {
        ast_error(token, "Expected opening parenthesis after identifier in call expression, got %s\n", token->value);
    }
    lexer_advance_cursor(lexer, 2);
    while (true) {
        token = lexer_peek_token(lexer, 0);
        if (token->type == TOKEN_PUNCTUATION && strcmp(token->value, ")") == 0) {
            break;
        }
        Node* argument = ast_parse_expression(lexer);
        if (argument == NULL) {
            break;
        } else {
            node_add_child(call_expression, argument);
        }
    }
    token = lexer_peek_token(lexer, 0);
    if (token->type != TOKEN_PUNCTUATION || strcmp(token->value, ")") != 0) {
        ast_error(token, "Expected closing parenthesis after call expression, got %s\n", token->value);
    }
    return call_expression;
}