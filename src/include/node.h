#pragma once

#include <stdlib.h>

typedef enum {
    NODE_PROGRAM,
    NODE_VARIABLE_DECLARATION,
    NODE_FUNCTION_DECLARATION,
    NODE_FUNCTION_ARGUMENT,
    NODE_ASSIGNMENT,
    NODE_IDENTIFIER,
    NODE_TYPE,
    NODE_BLOCK_STATEMENT,
    NODE_RETURN_STATEMENT,
    NODE_OPERATOR,
    NODE_EXPRESSION,
    NODE_IF_STATEMENT,
    NODE_NUMERIC_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_CALL_EXPRESSION,
    NODE_STRING_LITERAL,
    NODE_TRUE_LITERAL,
    NODE_FALSE_LITERAL,
    NODE_NULL_LITERAL,
    NODE_COMMENT,
} NodeType;

typedef struct Node Node;

typedef struct Node {
    NodeType type;
    void* data;
    Node** children;
    size_t num_children;
} Node;

Node* create_node(NodeType type, void* data);
void destroy_node(Node* node);
void node_add_child(Node* parent, Node* child);

void print_node(Node* node, int indent);
char* node_type_to_string(NodeType type);