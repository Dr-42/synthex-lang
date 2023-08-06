#include "node.h"

#include <locale.h>
#include <stdio.h>
#include <wchar.h>

#ifndef NOCOLOR
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RESET "\x1b[0m"
#else
#define ANSI_COLOR_RED ""
#define ANSI_COLOR_GREEN ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_RESET ""
#endif

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
    child->parent = parent;
}

void print_node(Node* node, bool* indents, int indent) {
    for (int i = 0; i < indent - 1; i++) {
        printf("%s%s%s", ANSI_COLOR_GREEN, indents[i] ? "| " : "  ", ANSI_COLOR_RESET);
    }
    if (indent > 0) {
        printf("%s%s%s", ANSI_COLOR_GREEN, "|_", ANSI_COLOR_RESET);
    }
    printf("%s%s%s", ANSI_COLOR_MAGENTA, node_type_to_string(node->type), ANSI_COLOR_RESET);
    if (node->data != NULL) {
        printf("%s %s%s", ANSI_COLOR_RED, (char*)node->data, ANSI_COLOR_RESET);
    }
    printf("\n");
    for (size_t i = 0; i < node->num_children; i++) {
        if (i < node->num_children - 1) {
            indents[indent] = true;
        } else {
            indents[indent] = false;
        }
        print_node(node->children[i], indents, indent + 1);
    }
}

char* node_type_to_string(NodeType type) {
    switch (type) {
        case NODE_PROGRAM:
            return "NODE_PROGRAM";
        case NODE_VARIABLE_DECLARATION:
            return "NODE_VARIABLE_DECLARATION";
        case NODE_ARRAY_DECLARATION:
            return "NODE_ARRAY_DECLARATION";
        case NODE_POINTER_DECLARATION:
            return "NODE_POINTER_DECLARATION";
        case NODE_POINTER_DEREF:
            return "NODE_POINTER_DEREF";
        case NODE_FUNCTION_DECLARATION:
            return "NODE_FUNCTION_DECLARATION";
        case NODE_FUNCTION_ARGUMENT:
            return "NODE_FUNCTION_ARGUMENT";
        case NODE_ASSIGNMENT:
            return "NODE_ASSIGNMENT";
        case NODE_ARRAY_ASSIGNMENT:
            return "NODE_ARRAY_ASSIGNMENT";
        case NODE_ARRAY_EXPRESSION:
            return "NODE_ARRAY_EXPRESSION";
        case NODE_ARRAY_ELEMENT:
            return "NODE_ARRAY_ELEMENT";
        case NODE_IDENTIFIER:
            return "NODE_IDENTIFIER";
        case NODE_TYPE:
            return "NODE_TYPE";
        case NODE_BLOCK_STATEMENT:
            return "NODE_BLOCK_STATEMENT";
        case NODE_RETURN_STATEMENT:
            return "NODE_RETURN_STATEMENT";
        case NODE_OPERATOR:
            return "NODE_OPERATOR";
        case NODE_EXPRESSION:
            return "NODE_EXPRESSION";
        case NODE_IF_STATEMENT:
            return "NODE_IF_STATEMENT";
        case NODE_ELIF_STATEMENT:
            return "NODE_ELIF_STATEMENT";
        case NODE_ELSE_STATEMENT:
            return "NODE_ELSE_STATEMENT";
        case NODE_WHILE_STATEMENT:
            return "NODE_WHILE_STATEMENT";
        case NODE_NUMERIC_LITERAL:
            return "NODE_NUMERIC_LITERAL";
        case NODE_FLOAT_LITERAL:
            return "NODE_FLOAT_LITERAL";
        case NODE_CALL_EXPRESSION:
            return "NODE_CALL_EXPRESSION";
        case NODE_STRING_LITERAL:
            return "NODE_STRING_LITERAL";
        case NODE_TRUE_LITERAL:
            return "NODE_TRUE_LITERAL";
        case NODE_FALSE_LITERAL:
            return "NODE_FALSE_LITERAL";
        case NODE_NULL_LITERAL:
            return "NODE_NULL_LITERAL";
        case NODE_COMMENT:
            return "NODE_COMMENT";
        default:
            return "UNKNOWN";
    }
}
