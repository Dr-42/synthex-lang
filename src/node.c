#include "node.h"

#include <stdio.h>

Node* create_node(NodeType type, void* data) {
    Node* node = calloc(1, sizeof(Node));
    node->type = type;
    node->data = data;
    node->children = NULL;
    node->num_children = 0;
    return node;
}

void destroy_node(Node* node) {
    for (size_t i = 0; i < node->num_children; i++) {
        destroy_node(node->children[i]);
    }
    free(node->children);
    free(node);
}

void node_add_child(Node* parent, Node* child) {
    parent->num_children++;
    parent->children = realloc(parent->children, parent->num_children * sizeof(Node*));
    parent->children[parent->num_children - 1] = child;
}

void print_node(Node* node, int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    printf("%s", node_type_to_string(node->type));
    if (node->data != NULL) {
        printf(" %s", (char*)node->data);
    }
    printf("\n");
    for (size_t i = 0; i < node->num_children; i++) {
        print_node(node->children[i], indent + 1);
    }
}

char* node_type_to_string(NodeType type) {
    switch (type) {
        case NODE_PROGRAM:
            return "NODE_PROGRAM";
        case NODE_VARIABLE_DECLARATION:
            return "NODE_VARIABLE_DECLARATION";
        case NODE_FUNCTION_DECLARATION:
            return "NODE_FUNCTION_DECLARATION";
        case NODE_IDENTIFIER:
            return "NODE_IDENTIFIER";
        case NODE_BLOCK_STATEMENT:
            return "NODE_BLOCK_STATEMENT";
        case NODE_RETURN_STATEMENT:
            return "NODE_RETURN_STATEMENT";
        case NODE_BINARY_EXPRESSION:
            return "NODE_BINARY_EXPRESSION";
        case NODE_IF_STATEMENT:
            return "NODE_IF_STATEMENT";
        case NODE_NUMERIC_LITERAL:
            return "NODE_NUMERIC_LITERAL";
        case NODE_CALL_EXPRESSION:
            return "NODE_CALL_EXPRESSION";
        case NODE_STRING_LITERAL:
            return "NODE_STRING_LITERAL";
        default:
            return "UNKNOWN";
    }
}
