#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    NODE_PROGRAM,
    NODE_VARIABLE_DECLARATION,
    NODE_ARRAY_DECLARATION,
    NODE_FUNCTION_DECLARATION,
    NODE_FUNCTION_ARGUMENT,
    NODE_ASSIGNMENT,
    NODE_ARRAY_ASSIGNMENT,
    NODE_ARRAY_EXPRESSION,
    NODE_ARRAY_ELEMENT,
    NODE_IDENTIFIER,
    NODE_TYPE,
    NODE_BLOCK_STATEMENT,
    NODE_RETURN_STATEMENT,
    NODE_OPERATOR,
    NODE_EXPRESSION,
    NODE_IF_STATEMENT,
    NODE_ELIF_STATEMENT,
    NODE_ELSE_STATEMENT,
    NODE_WHILE_STATEMENT,
    NODE_NUMERIC_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_CALL_EXPRESSION,
    NODE_STRING_LITERAL,
    NODE_TRUE_LITERAL,
    NODE_FALSE_LITERAL,
    NODE_NULL_LITERAL,
    NODE_BRK_STATEMENT,
    NODE_CONT_STATEMENT,
    NODE_COMMENT,
} NodeType;

typedef struct Node Node;

typedef struct Node {
    NodeType type;
    void* data;
    Node** children;
    Node* parent;
    size_t num_children;
} Node;

Node* create_node(NodeType type, void* data);
void destroy_node(Node* node);
void node_add_child(Node* parent, Node* child);

void print_node(Node* node, bool* indents, int indent);
char* node_type_to_string(NodeType type);