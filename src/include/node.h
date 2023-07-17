#pragma once

#include <stdlib.h>

typedef enum {
    NODE_PROGRAM,
    NODE_VARIABLE_DECLARATION,
    NODE_FUNCTION_DECLARATION,
    NODE_IDENTIFIER,
    NODE_BLOCK_STATEMENT,
    NODE_RETURN_STATEMENT,
    NODE_BINARY_EXPRESSION,
    NODE_IF_STATEMENT,
    NODE_NUMERIC_LITERAL,
    NODE_CALL_EXPRESSION,
    NODE_STRING_LITERAL
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