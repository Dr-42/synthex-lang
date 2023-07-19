#include "codegen.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char* types[];
extern const size_t TYPE_COUNT;

LLVMTypeRef* llvm_types;

LLVMTypeRef functionReturns[100] = {0};
const char* functionNames[100] = {0};
size_t functionCount = 0;

void convert_all_types(LLVMContextRef ctx) {
    llvm_types = calloc(TYPE_COUNT, sizeof(LLVMTypeRef));
    llvm_types[0] = LLVMInt8TypeInContext(ctx);
    llvm_types[1] = LLVMInt16TypeInContext(ctx);
    llvm_types[2] = LLVMInt32TypeInContext(ctx);
    llvm_types[3] = LLVMInt64TypeInContext(ctx);
    llvm_types[4] = LLVMFloatTypeInContext(ctx);
    llvm_types[5] = LLVMDoubleTypeInContext(ctx);
    llvm_types[6] = LLVMPointerType(LLVMInt8Type(), 0);
    llvm_types[7] = LLVMInt8TypeInContext(ctx);
    llvm_types[8] = LLVMInt1TypeInContext(ctx);
    llvm_types[9] = LLVMVoidTypeInContext(ctx);
    llvm_types[10] = LLVMPointerTypeInContext(ctx, 0);
}

void ast_to_llvm(AST* ast, Lexer* lexer) {
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext(lexer->filename, ctx);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

    convert_all_types(ctx);

    visit_node(ast->root, lexer, module, builder);

    char* error = NULL;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    // set target triple for module
    LLVMSetTarget(module, LLVMGetDefaultTargetTriple());

    // Assuming you have an LLVM module `module` and a function named "add" in it
    LLVMExecutionEngineRef engine;
    error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);

    // Emit .ll file
    char* ll_filename = calloc(strlen(lexer->filename) + 3, sizeof(char));
    strcpy(ll_filename, lexer->filename);
    strcat(ll_filename, ".ll");
    LLVMPrintModuleToFile(module, ll_filename, &error);
    if (error) {
        printf("Error: %s\n", error);
        LLVMDisposeMessage(error);
    }
    free(ll_filename);

    LLVMDisposeExecutionEngine(engine);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(ctx);
}

void visit_node(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMValueRef lhs = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        switch (child->type) {
            case NODE_PROGRAM:
                visit_node_program(child, lexer, module, builder);
                break;
            case NODE_VARIABLE_DECLARATION:
                visit_node_variable_declaration(child, lexer, module, builder);
                break;
            case NODE_FUNCTION_DECLARATION:
                visit_node_function_declaration(child, lexer, module, builder);
                break;
            case NODE_FUNCTION_ARGUMENT:
                visit_node_function_argument(child, lexer, module, builder);
                break;
            case NODE_ASSIGNMENT:
                visit_node_assignment(child, lexer, module, builder);
                break;
            case NODE_IDENTIFIER:
                lhs = visit_node_identifier(child, lexer, module, builder);
                continue;
            case NODE_TYPE:
                visit_node_type(child, lexer, module, builder);
                break;
            case NODE_BLOCK_STATEMENT:
                break;
            case NODE_RETURN_STATEMENT:
                // visit_node_return_statement(child, lexer, module, builder);
                break;
            case NODE_OPERATOR:
                Node* rhs;
                rhs = node->children[i + 1];

                LLVMValueRef value2 = visit_node_expression(rhs, lexer, module, builder);
                visit_node_operator(child, lexer, module, builder, lhs, value2);
                break;
            case NODE_EXPRESSION:
                visit_node_expression(child, lexer, module, builder);
                break;
            case NODE_IF_STATEMENT:
                // TODO:
                break;
            case NODE_NUMERIC_LITERAL:
                visit_node_numeric_literal(child, lexer, module, builder);
                break;
            case NODE_FLOAT_LITERAL:
                visit_node_float_literal(child, lexer, module, builder);
                break;
            case NODE_CALL_EXPRESSION:
                visit_node_call_expression(child, lexer, module, builder);
                break;
            case NODE_STRING_LITERAL:
                visit_node_string_literal(child, lexer, module, builder);
                break;
            case NODE_TRUE_LITERAL:
                visit_node_true_literal(child, lexer, module, builder);
                break;
            case NODE_FALSE_LITERAL:
                visit_node_false_literal(child, lexer, module, builder);
                break;
            case NODE_NULL_LITERAL:
                visit_node_null_literal(child, lexer, module, builder);
                break;
            case NODE_COMMENT:
                break;
            default:
                printf("Unknown node type: %s\n", node_type_to_string(node->type));
                break;
        }
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
    char** arg_names = NULL;
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
        arg_names = calloc(arg_count, sizeof(char*));
        size_t arg_index = 0;
        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_FUNCTION_ARGUMENT) {
                Node* type_node = child->children[0];
                for (size_t j = 0; j < TYPE_COUNT; j++) {
                    if (strcmp(type_node->data, types[j]) == 0) {
                        arg_types[arg_index] = llvm_types[j];  // Corrected line
                        arg_names[arg_index] = child->data;    // Corrected line
                        arg_index++;                           // Increment the index after assigning values
                        break;
                    }
                }
            }
        }

        LLVMTypeRef func_type = LLVMFunctionType(return_type, arg_types, arg_count, 0);
        LLVMValueRef func = LLVMAddFunction(module, func_name, func_type);

        functionNames[functionCount] = func_name;
        functionReturns[functionCount] = return_type;
        functionCount++;
        for (size_t i = 0; i < arg_count; i++) {
            LLVMValueRef arg = LLVMGetParam(func, i);
            LLVMSetValueName2(arg, arg_names[i], strlen(arg_names[i]));
        }

        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_BLOCK_STATEMENT) {
                visit_node_block_statement(child, lexer, module, builder, func, return_type);
            }
        }

        LLVMDumpValue(func);
    }
    for (size_t i = 0; i < arg_count; i++) {
        free(arg_names[i]);
    }
    free(arg_names);
}

void visit_node_function_argument(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

LLVMValueRef visit_node_assignment(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return NULL;
}

LLVMValueRef visit_node_identifier(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    const char* identifier = node->data;
    LLVMBasicBlockRef currentBlock = LLVMGetInsertBlock(builder);
    LLVMValueRef currentFunction = LLVMGetBasicBlockParent(currentBlock);
    unsigned int paramCount = LLVMCountParams(currentFunction);

    LLVMValueRef value = NULL;

    for (unsigned int i = 0; i < paramCount; i++) {
        LLVMValueRef param = LLVMGetParam(currentFunction, i);
        const char* param_name = LLVMGetValueName(param);
        if (strcmp(param_name, identifier) == 0) {
            value = param;
            break;
        }
    }

    if (value == NULL) {
        printf("Error: Variable '%s' not found\n", identifier);
        return NULL;
    }
    LLVMValueRef variable = value;
    return variable;
}

void visit_node_type(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_block_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type) {
    LLVMContextRef ctx = LLVMGetModuleContext(module);
    LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(ctx, func, "entry");
    LLVMBuilderRef block_builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(block_builder, block);
    LLVMValueRef return_value = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_RETURN_STATEMENT) {
            return_value = visit_node_return_statement(node->children[i], lexer, module, block_builder, return_type);
        } else {
            visit_node(node->children[i], lexer, module, block_builder);
        }
    }
    if (return_value != NULL) {
        LLVMBuildRet(block_builder, return_value);
    }
    LLVMDisposeBuilder(block_builder);
}

LLVMValueRef visit_node_return_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMTypeRef return_type) {
    LLVMValueRef value = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_EXPRESSION) {
            value = visit_node_expression(node->children[i], lexer, module, builder);
        } else if (node->children[i]->type == NODE_IDENTIFIER) {
            value = visit_node_identifier(node->children[i], lexer, module, builder);
        }
    }
    return value;
}

LLVMValueRef visit_node_operator(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef value1, LLVMValueRef value2) {
    const char* op = node->data;
    if (strcmp(op, "+") == 0) {
        return LLVMBuildAdd(builder, value1, value2, "addtmp");
    } else if (strcmp(op, "-") == 0) {
        return LLVMBuildSub(builder, value1, value2, "subtmp");
    } else if (strcmp(op, "*") == 0) {
        return LLVMBuildMul(builder, value1, value2, "multmp");
    } else {
        printf("Error: Unsupported operator '%s'\n", op);
    }

    return NULL;
}

LLVMValueRef visit_node_expression(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMValueRef lhs = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        switch (child->type) {
            case NODE_IDENTIFIER:
                lhs = visit_node_identifier(child, lexer, module, builder);
                continue;
            case NODE_OPERATOR:
                Node* rhs;
                rhs = node->children[i + 1];

                LLVMValueRef value2 = visit_node_identifier(rhs, lexer, module, builder);
                return visit_node_operator(child, lexer, module, builder, lhs, value2);
                break;
            case NODE_CALL_EXPRESSION:
                return visit_node_call_expression(child, lexer, module, builder);
                break;
            case NODE_NUMERIC_LITERAL:
                return visit_node_numeric_literal(child, lexer, module, builder);
                break;
            case NODE_FLOAT_LITERAL:
                return visit_node_float_literal(child, lexer, module, builder);
                break;
            case NODE_TRUE_LITERAL:
                return visit_node_true_literal(child, lexer, module, builder);
                break;
            case NODE_FALSE_LITERAL:
                return visit_node_false_literal(child, lexer, module, builder);
                break;
            case NODE_NULL_LITERAL:
                return visit_node_null_literal(child, lexer, module, builder);
                break;
            default:
                printf("Unknown expression node type: %s\n", node_type_to_string(node->type));
                break;
        }
    }
}

LLVMValueRef visit_node_call_expression(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    const char* function_name = node->children[0]->data;
    LLVMValueRef function = LLVMGetNamedFunction(module, function_name);
    if (function == NULL) {
        printf("Error: Function '%s' not found\n", function_name);
        return NULL;
    }

    // Get number of parameters
    size_t param_count = LLVMCountParams(function);
    // Allocate memory for parameter types
    LLVMTypeRef* param_types = calloc(param_count, sizeof(LLVMTypeRef));
    // Get each parameter type
    for (size_t i = 0; i < param_count; ++i) {
        LLVMValueRef param = LLVMGetParam(function, i);
        param_types[i] = LLVMTypeOf(param);
    }
    LLVMTypeRef ret_type = NULL;

    // Get return type
    for (size_t i = 0; i < functionCount; i++) {
        if (strcmp(functionNames[i], function_name) == 0) {
            ret_type = functionReturns[i];
        }
    }

    if (ret_type == NULL) {
        printf("Error: Function '%s' not found\n", function_name);
        return NULL;
    }

    // Create function type
    LLVMTypeRef function_type = LLVMFunctionType(ret_type, param_types, param_count, false);

    // Validate against node's number of children - 1 (function name)
    if (node->num_children - 1 != param_count) {
        printf("Error: Incorrect number of arguments for function '%s'\n", function_name);
        return NULL;
    }

    size_t num_args = node->num_children - 1;
    LLVMValueRef* args = (LLVMValueRef*)malloc(num_args * sizeof(LLVMValueRef));
    size_t arg_count = 0;
    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_EXPRESSION) {
            args[arg_count] = visit_node_expression(node->children[i], lexer, module, builder);
            arg_count++;
        }
    }

    LLVMValueRef ret = LLVMBuildCall2(builder, function_type, function, args, num_args, "calltmp");
    free(param_types);  // free parameter types
    free(args);
    return ret;
}

void visit_node_string_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

LLVMValueRef visit_node_numeric_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMContextRef ctx = LLVMGetModuleContext(module);
    const char* value_str = node->data;
    float value = strtof(value_str, NULL);
    return LLVMConstInt(LLVMInt32TypeInContext(ctx), value, 0);
}

LLVMValueRef visit_node_float_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    const char* value_str = node->data;
    float value = strtof(value_str, NULL);
    return LLVMConstReal(LLVMFloatType(), value);
}

LLVMValueRef visit_node_true_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return LLVMConstInt(LLVMInt1Type(), 1, 0);
}

LLVMValueRef visit_node_false_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return LLVMConstInt(LLVMInt1Type(), 0, 0);
}

LLVMValueRef visit_node_null_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return LLVMConstPointerNull(LLVMPointerType(LLVMInt8Type(), 0));
}
