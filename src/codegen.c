#include "codegen.h"

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char* types[];
extern const size_t TYPE_COUNT;

LLVMTypeRef* llvm_types;

void convert_all_types() {
    llvm_types = calloc(TYPE_COUNT, sizeof(LLVMTypeRef));
    llvm_types[0] = LLVMInt8Type();
    llvm_types[1] = LLVMInt16Type();
    llvm_types[2] = LLVMInt32Type();
    llvm_types[3] = LLVMInt64Type();
    llvm_types[4] = LLVMFloatType();
    llvm_types[5] = LLVMDoubleType();
    llvm_types[6] = LLVMPointerType(LLVMInt8Type(), 0);
    llvm_types[7] = LLVMInt8Type();
    llvm_types[8] = LLVMInt1Type();
    llvm_types[9] = LLVMVoidType();
    llvm_types[10] = LLVMPointerType(LLVMInt8Type(), 0);
}

void ast_to_llvm(AST* ast, Lexer* lexer) {
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext(lexer->filename, ctx);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

    convert_all_types();

    visit_node(ast->root, lexer, module, builder);

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(ctx);
}

void visit_node(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    switch (node->type) {
        case NODE_PROGRAM:
            visit_node_program(node, lexer, module, builder);
            break;
        case NODE_VARIABLE_DECLARATION:
            visit_node_variable_declaration(node, lexer, module, builder);
            break;
        case NODE_FUNCTION_DECLARATION:
            visit_node_function_declaration(node, lexer, module, builder);
            break;
        case NODE_FUNCTION_ARGUMENT:
            visit_node_function_argument(node, lexer, module, builder);
            break;
        case NODE_ASSIGNMENT:
            visit_node_assignment(node, lexer, module, builder);
            break;
        case NODE_IDENTIFIER:
            visit_node_identifier(node, lexer, module, builder);
            break;
        case NODE_TYPE:
            visit_node_type(node, lexer, module, builder);
            break;
        case NODE_BLOCK_STATEMENT:
            visit_node_block_statement(node, lexer, module, builder);
            break;
        case NODE_RETURN_STATEMENT:
            visit_node_return_statement(node, lexer, module, builder);
            break;
        case NODE_OPERATOR:
            visit_node_operator(node, lexer, module, builder);
            break;
        case NODE_EXPRESSION:
            visit_node_expression(node, lexer, module, builder);
            break;
        case NODE_IF_STATEMENT:
            // TODO:
            break;
        case NODE_NUMERIC_LITERAL:
            visit_node_numeric_literal(node, lexer, module, builder);
            break;
        case NODE_FLOAT_LITERAL:
            visit_node_float_literal(node, lexer, module, builder);
            break;
        case NODE_CALL_EXPRESSION:
            visit_node_call_expression(node, lexer, module, builder);
            break;
        case NODE_STRING_LITERAL:
            visit_node_string_literal(node, lexer, module, builder);
            break;
        case NODE_TRUE_LITERAL:
            visit_node_true_literal(node, lexer, module, builder);
            break;
        case NODE_FALSE_LITERAL:
            visit_node_false_literal(node, lexer, module, builder);
            break;
        case NODE_NULL_LITERAL:
            visit_node_null_literal(node, lexer, module, builder);
            break;
        case NODE_COMMENT:
            break;
        default:
            printf("Unknown node type: %s\n", node_type_to_string(node->type));
            break;
    }
}

void visit_node_program(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    for (size_t i = 0; i < node->num_children; i++) {
        visit_node(node->children[i], lexer, module, builder);
    }
}

void visit_node_variable_declaration(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMTypeRef type;
    char* var_name = NULL;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            var_name = child->data;
            Node* type_node = child->children[0];
            for (size_t j = 0; j < TYPE_COUNT; j++) {
                if (strcmp(type_node->data, types[j]) == 0) {
                    type = llvm_types[j];
                    break;
                }
            }
        }
    }

    if (var_name != NULL) {
        LLVMValueRef var = LLVMBuildAlloca(builder, type, var_name);
        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_EXPRESSION) {
                visit_node(child, lexer, module, builder);
                LLVMValueRef value = LLVMBuildLoad2(builder, type, var, var_name);
                LLVMBuildStore(builder, value, var);
            }
        }
        LLVMValueRef value = LLVMBuildLoad2(builder, type, var, var_name);
        LLVMDumpValue(value);
    }
}

void visit_node_function_declaration(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    char* func_name = NULL;
    LLVMTypeRef return_type = NULL;
    LLVMTypeRef* arg_types = NULL;
    size_t arg_count = 0;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            func_name = child->data;
        } else if (child->type == NODE_TYPE) {
            for (size_t j = 0; j < TYPE_COUNT; j++) {
                if (strcmp(child->data, types[j]) == 0) {
                    return_type = llvm_types[j];
                    break;
                }
            }
        } else if (child->type == NODE_FUNCTION_ARGUMENT) {
            arg_count++;
        }
    }

    if (func_name != NULL && return_type != NULL) {
        arg_types = calloc(arg_count, sizeof(LLVMTypeRef));
        size_t arg_index = 0;
        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_FUNCTION_ARGUMENT) {
                Node* type_node = child->children[0];
                for (size_t j = 0; j < TYPE_COUNT; j++) {
                    if (strcmp(type_node->data, types[j]) == 0) {
                        arg_types[arg_index++] = llvm_types[j];
                        break;
                    }
                }
            }
        }

        LLVMTypeRef func_type = LLVMFunctionType(return_type, arg_types, arg_count, 0);
        LLVMValueRef func = LLVMAddFunction(module, func_name, func_type);

        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_BLOCK_STATEMENT) {
                visit_node(child, lexer, module, builder);
            }
        }

        LLVMDumpValue(func);
    }

    free(arg_types);
}

void visit_node_function_argument(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_assignment(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_identifier(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_type(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_block_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    for (size_t i = 0; i < node->num_children; i++) {
        visit_node(node->children[i], lexer, module, builder);
    }
}

void visit_node_return_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_operator(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_expression(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_numeric_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_float_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_call_expression(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_string_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_true_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_false_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_null_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}
