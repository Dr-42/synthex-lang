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
LLVMValueRef current_function = NULL;

LLVMValueRef current_scope_variables[100] = {0};
LLVMTypeRef current_scope_variable_types[100] = {0};
const char* current_scope_variable_names[100] = {0};
size_t current_scope_variable_count = 0;

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
    char* ll_filename = strcat(lexer->filename, ".ll");
    LLVMPrintModuleToFile(module, ll_filename, &error);
    if (error) {
        printf("Error: %s\n", error);
        LLVMDisposeMessage(error);
    }

    LLVMRemoveModule(engine, module, &module, &error);
    if (error) {
        printf("Error: %s\n", error);
        LLVMDisposeMessage(error);
    }

    LLVMDisposeExecutionEngine(engine);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(ctx);
}

LLVMValueRef visit_node(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMValueRef lhs = NULL;
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
            return visit_node_identifier(node, lexer, module, builder, true);
            break;
        case NODE_TYPE:
            visit_node_type(node, lexer, module, builder);
            break;
        case NODE_BLOCK_STATEMENT:
            break;
        case NODE_RETURN_STATEMENT:
            // visit_node_return_statement(node, lexer, module, builder);
            break;
        case NODE_OPERATOR:
            break;
        case NODE_EXPRESSION:
            return visit_node_expression(node, lexer, module, builder);
            break;
        case NODE_IF_STATEMENT:
            // TODO:
            break;
        case NODE_NUMERIC_LITERAL:
            return visit_node_numeric_literal(node, lexer, module, builder);
            break;
        case NODE_FLOAT_LITERAL:
            return visit_node_float_literal(node, lexer, module, builder);
            break;
        case NODE_CALL_EXPRESSION:
            return visit_node_call_expression(node, lexer, module, builder);
            break;
        case NODE_STRING_LITERAL:
            visit_node_string_literal(node, lexer, module, builder);
            break;
        case NODE_TRUE_LITERAL:
            return visit_node_true_literal(node, lexer, module, builder);
            break;
        case NODE_FALSE_LITERAL:
            return visit_node_false_literal(node, lexer, module, builder);
            break;
        case NODE_NULL_LITERAL:
            return visit_node_null_literal(node, lexer, module, builder);
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

    var_name = node->data;
    for (size_t i = 0; i < TYPE_COUNT; i++) {
        if (strcmp(node->children[0]->data, types[i]) == 0) {
            type = llvm_types[i];
            break;
        }
    }

    if (var_name != NULL) {
        // Allocate variable
        LLVMValueRef variable = LLVMBuildAlloca(builder, type, var_name);
        //  Add variable to current scope
        current_scope_variables[current_scope_variable_count] = variable;
        current_scope_variable_names[current_scope_variable_count] = var_name;
        current_scope_variable_types[current_scope_variable_count] = type;
        current_scope_variable_count++;
    } else {
        printf("Error: Variable '%s' could not be declared\n", var_name);
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

        current_scope_variable_count = 0;

        for (size_t i = 0; i < arg_count; i++) {
            LLVMValueRef arg = LLVMGetParam(func, i);
            LLVMSetValueName2(arg, arg_names[i], strlen(arg_names[i]));
        }

        current_function = func;

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

LLVMBasicBlockRef create_if_block(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type, const char* name) {
    LLVMContextRef ctx = LLVMGetModuleContext(module);
    LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(ctx, func, name);
    LLVMBuilderRef block_builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(block_builder, block);

    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_RETURN_STATEMENT) {
            LLVMValueRef return_value = visit_node_return_statement(node->children[i], lexer, module, block_builder, return_type);
            LLVMBuildRet(block_builder, return_value);
        } else if (node->children[i]->type == NODE_VARIABLE_DECLARATION) {
            visit_node_variable_declaration(node->children[i], lexer, module, block_builder);
        } else {
            visit_node(node->children[i], lexer, module, block_builder);
        }
    }

    return block;
}

void visit_node_if_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type) {
    LLVMValueRef condition = NULL;
    LLVMBasicBlockRef if_block = NULL;
    LLVMBasicBlockRef else_block = NULL;
    LLVMBasicBlockRef merge_block = NULL;

    LLVMBasicBlockRef elif_blocks[100] = {0};
    LLVMBasicBlockRef elif_cond_blocks[100] = {0};
    LLVMValueRef elif_conditions[100] = {0};
    size_t elif_count = 0;

    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_EXPRESSION) {
            condition = visit_node_expression(node->children[i], lexer, module, builder);
        } else if (node->children[i]->type == NODE_BLOCK_STATEMENT) {
            if_block = create_if_block(node->children[i], lexer, module, builder, func, return_type, "if");
        } else if (node->children[i]->type == NODE_ELSE_STATEMENT) {
            else_block = create_if_block(node->children[i]->children[0], lexer, module, builder, func, return_type, "else");
        } else if (node->children[i]->type == NODE_ELIF_STATEMENT) {
            Node* elif_node = node->children[i];
            for (size_t j = 0; j < elif_node->num_children; j++) {
                if (elif_node->children[j]->type == NODE_EXPRESSION) {
                    elif_conditions[elif_count] = visit_node_expression(elif_node->children[j], lexer, module, builder);
                } else if (elif_node->children[j]->type == NODE_BLOCK_STATEMENT) {
                    elif_blocks[elif_count] = create_if_block(elif_node->children[j], lexer, module, builder, func, return_type, "elif");
                }
            }
            elif_count++;
        }
    }

    if (condition != NULL && if_block != NULL) {
        merge_block = LLVMAppendBasicBlockInContext(LLVMGetModuleContext(module), func, "merge");

        if (else_block == NULL) {
            else_block = merge_block;
        }

        for (size_t i = 0; i < elif_count; i++) {
            elif_cond_blocks[i] = LLVMCreateBasicBlockInContext(LLVMGetModuleContext(module), "elif_cond");
        }

        // Position builder at end of the function block
        LLVMPositionBuilderAtEnd(builder, LLVMGetInsertBlock(builder));
        if (elif_count > 0) {
            for (size_t i = 0; i < elif_count; i++) {
                if (i == 0) {
                    LLVMBuildCondBr(builder, condition, if_block, elif_cond_blocks[i]);
                } else {
                    // LLVMBuildCondBr(builder, elif_conditions[i], elif_blocks[i], elif_cond_blocks[i]);
                }
                LLVMAppendExistingBasicBlock(func, elif_cond_blocks[i]);
                LLVMPositionBuilderAtEnd(builder, elif_cond_blocks[i]);
                if (i == elif_count - 1) {
                    LLVMBuildCondBr(builder, elif_conditions[i], elif_blocks[i], else_block);
                } else {
                    LLVMBuildCondBr(builder, elif_conditions[i], elif_blocks[i], elif_cond_blocks[i + 1]);
                }
            }
        } else {
            LLVMBuildCondBr(builder, condition, if_block, else_block);
        }
        // Position builder at end of the if block to add the merge block
        LLVMPositionBuilderAtEnd(builder, if_block);
        LLVMBuildBr(builder, merge_block);

        if (elif_count > 0) {
            for (size_t i = 0; i < elif_count; i++) {
                LLVMPositionBuilderAtEnd(builder, elif_blocks[i]);
                LLVMBuildBr(builder, merge_block);
            }
        }

        if (else_block != merge_block) {
            // Position builder at end of the else block to add the merge block
            LLVMPositionBuilderAtEnd(builder, else_block);
            LLVMBuildBr(builder, merge_block);
        }
        LLVMPositionBuilderAtEnd(builder, merge_block);
    }

    return;
}

void visit_node_assignment(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMValueRef value = NULL;
    LLVMValueRef variable = NULL;
    bool is_also_declaration = false;
    Node* declaration_node = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            if (child->num_children > 0) {
                if (child->children[0]->type == NODE_TYPE) {
                    is_also_declaration = true;
                    break;
                }
            }
        }
    }

    if (is_also_declaration) {
        declaration_node = create_node(NODE_VARIABLE_DECLARATION, NULL);
        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_IDENTIFIER) {
                declaration_node->data = child->data;
                declaration_node->type = NODE_VARIABLE_DECLARATION;
                Node* type_node = create_node(NODE_TYPE, child->children[0]->data);
                node_add_child(declaration_node, type_node);
            }
        }
        visit_node_variable_declaration(declaration_node, lexer, module, builder);
        destroy_node(declaration_node);
    }

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            variable = visit_node_identifier(child, lexer, module, builder, false);
        } else if (child->type == NODE_EXPRESSION) {
            value = visit_node_expression(child, lexer, module, builder);
        }
    }

    if (variable != NULL && value != NULL) {
        LLVMTypeRef variable_type = LLVMTypeOf(variable);
        LLVMTypeRef value_type = LLVMTypeOf(value);

        // printf("Variable type: %s\n", LLVMPrintTypeToString(variable_type));
        // printf("Value type: %s\n", LLVMPrintTypeToString(value_type));

        LLVMBuildStore(builder, value, variable);
    } else {
        printf("Error: Variable '%s' could not be assigned\n", node->data);
    }
}

LLVMValueRef visit_node_identifier(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, bool deref) {
    const char* identifier = node->data;
    // LLVMBasicBlockRef currentBlock = LLVMGetInsertBlock(builder);
    LLVMValueRef currentFunction = current_function;
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

    // Check if variable is in the current scope
    if (value == NULL) {
        for (int i = 0; i < current_scope_variable_count; i++) {
            if (strcmp(current_scope_variable_names[i], identifier) == 0) {
                value = current_scope_variables[i];
                if (deref) {
                    value = LLVMBuildLoad2(builder, current_scope_variable_types[i], value, identifier);
                }
                break;
            }
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
        } else if (node->children[i]->type == NODE_VARIABLE_DECLARATION) {
            visit_node_variable_declaration(node->children[i], lexer, module, block_builder);
        } else if (node->children[i]->type == NODE_IF_STATEMENT) {
            visit_node_if_statement(node->children[i], lexer, module, block_builder, func, return_type);
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
            value = visit_node_identifier(node->children[i], lexer, module, builder, true);
        }
    }
    return value;
}

LLVMValueRef visit_node_operator(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef value1, LLVMValueRef value2) {
    const char* op = node->data;

    if (value1 == NULL || value2 == NULL) {
        printf("Error: Operator '%s' could not be applied\n", op);
        return NULL;
    }

    LLVMTypeRef value1_type = LLVMTypeOf(value1);
    LLVMTypeRef value2_type = LLVMTypeOf(value2);

    if (LLVMGetTypeKind(value1_type) != LLVMGetTypeKind(value2_type)) {
        printf("Error: Operator '%s' could not be applied to different types\n", op);
        return NULL;
    }

    if (LLVMGetTypeKind(value1_type) == LLVMIntegerTypeKind) {
        if (strcmp(op, "+") == 0) {
            return LLVMBuildAdd(builder, value1, value2, "addtmp");
        } else if (strcmp(op, "-") == 0) {
            return LLVMBuildSub(builder, value1, value2, "subtmp");
        } else if (strcmp(op, "*") == 0) {
            return LLVMBuildMul(builder, value1, value2, "multmp");
        } else if (strcmp(op, "==") == 0) {
            return LLVMBuildICmp(builder, LLVMIntEQ, value1, value2, "eqtmp");
        } else if (strcmp(op, "!-") == 0) {
            return LLVMBuildICmp(builder, LLVMIntNE, value1, value2, "neqtmp");
        } else if (strcmp(op, "<") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSLT, value1, value2, "lttmp");
        } else if (strcmp(op, ">") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSGT, value1, value2, "gttmp");
        } else if (strcmp(op, "<=") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSLE, value1, value2, "letmp");
        } else if (strcmp(op, ">=") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSGE, value1, value2, "getmp");
        } else {
            printf("Error: Unsupported operator '%s'\n", op);
        }
    } else if (LLVMGetTypeKind(value1_type) == LLVMFloatTypeKind) {
        if (strcmp(op, "+") == 0) {
            return LLVMBuildFAdd(builder, value1, value2, "addtmp");
        } else if (strcmp(op, "-") == 0) {
            return LLVMBuildFSub(builder, value1, value2, "subtmp");
        } else if (strcmp(op, "*") == 0) {
            return LLVMBuildFMul(builder, value1, value2, "multmp");
        } else {
            printf("Error: Unsupported operator '%s'\n", op);
        }
    } else {
        printf("Error: Unsupported type %s\n", LLVMPrintTypeToString(value1_type));
    }

    return NULL;
}

LLVMValueRef visit_node_expression(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMValueRef lhs = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        switch (child->type) {
            case NODE_EXPRESSION:
                lhs = visit_node_expression(child, lexer, module, builder);
                continue;
            case NODE_IDENTIFIER:
                lhs = visit_node_identifier(child, lexer, module, builder, true);
                continue;
            case NODE_OPERATOR:
                Node* rhs;
                rhs = node->children[i + 1];
                LLVMValueRef value2 = visit_node(rhs, lexer, module, builder);
                lhs = visit_node_operator(child, lexer, module, builder, lhs, value2);
                i++;
                continue;
            case NODE_CALL_EXPRESSION:
                lhs = visit_node_call_expression(child, lexer, module, builder);
                continue;
            case NODE_NUMERIC_LITERAL:
                lhs = visit_node_numeric_literal(child, lexer, module, builder);
                continue;
            case NODE_FLOAT_LITERAL:
                lhs = visit_node_float_literal(child, lexer, module, builder);
                continue;
            case NODE_TRUE_LITERAL:
                lhs = visit_node_true_literal(child, lexer, module, builder);
                continue;
            case NODE_FALSE_LITERAL:
                lhs = visit_node_false_literal(child, lexer, module, builder);
                continue;
            case NODE_NULL_LITERAL:
                lhs = visit_node_null_literal(child, lexer, module, builder);
                continue;
            default:
                printf("Unknown expression node type: %s\n", node_type_to_string(node->type));
                break;
        }
    }
    return lhs;
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
