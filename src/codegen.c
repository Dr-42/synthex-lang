#include "codegen.h"

#include <assert.h>
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
bool functionIsVararg[100] = {0};
size_t functionCount = 0;
LLVMValueRef current_function = NULL;

LLVMValueRef current_scope_variables[100] = {0};
LLVMTypeRef current_scope_variable_types[100] = {0};
const char* current_scope_variable_names[100] = {0};
size_t current_scope_variable_count = 0;

LLVMValueRef current_scope_arrays[100] = {0};
LLVMTypeRef current_scope_array_types[100] = {0};
LLVMTypeRef current_scope_array_element_types[100] = {0};
const char* current_scope_array_names[100] = {0};
size_t current_scope_array_dims[100] = {0};
size_t current_scope_array_count = 0;

LLVMValueRef current_scope_pointers[100] = {0};
LLVMTypeRef current_scope_pointer_types[100] = {0};
const char* current_scope_pointer_names[100] = {0};
LLVMTypeRef current_scope_pointer_base_types[100] = {0};
size_t current_scope_pointer_count = 0;

LLVMBasicBlockRef while_merge_block = NULL;
LLVMBasicBlockRef while_cond_block = NULL;

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
    llvm_types[10] = NULL;
}

void ast_to_llvm(AST* ast, Lexer* lexer) {
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMContextSetOpaquePointers(ctx, false);
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext(lexer->filename, ctx);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

    convert_all_types(ctx);

    visit_node(ast->root, lexer, module, builder);

    char* error = NULL;
    LLVMDumpModule(module);
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
    // delete the file if exists
    remove(ll_filename);
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
        case NODE_POINTER_DECLARATION:
            visit_node_pointer_declaration(node, lexer, module, builder);
            break;
        case NODE_POINTER_DEREF:
            visit_node_pointer_deref(node, lexer, module, builder);
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
            break;
        case NODE_OPERATOR:
            break;
        case NODE_EXPRESSION:
            return visit_node_expression(node, lexer, module, builder);
            break;
        case NODE_ARRAY_DECLARATION:
            visit_node_array_declaration(node, lexer, module, builder);
            break;
        case NODE_ARRAY_ASSIGNMENT:
            visit_node_array_assignment(node, lexer, module, builder);
            break;
        case NODE_ARRAY_ELEMENT:
            return visit_node_array_element(node, lexer, module, builder);
            break;
        case NODE_IF_STATEMENT:
            visit_node_if_statement(node, lexer, module, builder, current_function, functionReturns[functionCount - 1]);
            break;
        case NODE_WHILE_STATEMENT:
            visit_node_while_statement(node, lexer, module, builder, current_function, functionReturns[functionCount - 1]);
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
        case NODE_BRK_STATEMENT:
            if (while_merge_block != NULL && while_cond_block != NULL) {
                LLVMBuildBr(builder, while_merge_block);
            } else {
                fprintf(stderr, "Error: Break statement outside of loop\n");
            }
            break;
        case NODE_CONT_STATEMENT:
            if (while_cond_block != NULL && while_merge_block != NULL) {
                LLVMBuildBr(builder, while_cond_block);
            } else {
                fprintf(stderr, "Error: Continue statement outside of loop\n");
            }
            break;
        default:
            printf("Unknown node type: %s\n", node_type_to_string(node->type));
            break;
    }

    return NULL;
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
    bool is_vararg = false;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            func_name = child->data;
        } else if (child->type == NODE_TYPE) {
            for (size_t j = 0; j < TYPE_COUNT; j++) {
                if (strcmp(child->data, types[j]) == 0) {
                    if (strcmp(child->data, "ptr") == 0) {
                        size_t pointer_degree = 1;
                        Node* type_node = child->children[0];
                        bool is_ptr = false;
                        while (!is_ptr) {
                            if (strcmp(type_node->data, "ptr") == 0) {
                                type_node = type_node->children[0];
                                pointer_degree++;
                            } else {
                                is_ptr = true;
                            }
                        }

                        LLVMTypeRef type = NULL;
                        for (size_t j = 0; j < TYPE_COUNT; j++) {
                            if (strcmp(type_node->data, types[j]) == 0) {
                                type = llvm_types[j];
                                break;
                            }
                        }
                        if (type == NULL) {
                            fprintf(stderr, "Error: Pointer '%s' could not be declared due to empty base data type\n", func_name);
                            return;
                        }

                        for (size_t i = 0; i < pointer_degree; i++) {
                            type = LLVMPointerType(type, 0);
                        }
                        return_type = type;
                    } else {
                        return_type = llvm_types[j];
                    }
                    break;
                }
            }
        } else if (child->type == NODE_FUNCTION_ARGUMENT) {
            if (strcmp(child->data, "...") == 0) {
                is_vararg = true;
                continue;
            }
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
                if (strcmp(child->data, "...") == 0) {
                    functionIsVararg[functionCount] = true;
                    arg_types[arg_index] = LLVMPointerType(LLVMVoidType(), 0);
                    arg_names[arg_index] = child->data;
                    arg_index++;
                    continue;
                }
                Node* type_node = child->children[0];
                if (strcmp(type_node->data, "ptr") == 0) {
                    LLVMTypeRef base_data_type = NULL;
                    size_t pointer_degree = 0;
                    LLVMTypeRef type;
                    bool is_ptr = false;
                    while (!is_ptr) {
                        if (strcmp(type_node->data, "ptr") == 0) {
                            type_node = type_node->children[0];
                            pointer_degree++;
                        } else {
                            is_ptr = true;
                        }
                    }

                    for (size_t j = 0; j < TYPE_COUNT; j++) {
                        if (strcmp(type_node->data, types[j]) == 0) {
                            base_data_type = llvm_types[j];
                            break;
                        }
                    }
                    if (base_data_type == NULL) {
                        fprintf(stderr, "Error: Pointer '%s' could not be declared due to empty base data type\n", func_name);
                        return;
                    }

                    type = base_data_type;
                    LLVMTypeRef base_type = NULL;
                    for (size_t i = 0; i < pointer_degree; i++) {
                        base_type = type;
                        type = LLVMPointerType(type, 0);
                    }

                    arg_types[arg_index] = type;
                    arg_names[arg_index] = child->data;
                    arg_index++;
                } else {
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
        }

        LLVMTypeRef func_type = LLVMFunctionType(return_type, arg_types, arg_count, is_vararg);
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
        free(arg_names);
    }
}

void visit_node_pointer_declaration(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMTypeRef type;
    char* var_name = NULL;
    LLVMTypeRef base_data_type = NULL;
    size_t pointer_degree = 1;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            var_name = child->data;
        } else if (child->type == NODE_TYPE) {
            assert(strcmp(child->data, "ptr") == 0);
            // Find the pointer type. The child of the pointer type node is the type
            // If the child is a ptr type, then the child of the ptr type node is the type and so on
            Node* type_node = child->children[0];
            bool is_ptr = false;
            while (!is_ptr) {
                if (strcmp(type_node->data, "ptr") == 0) {
                    type_node = type_node->children[0];
                    pointer_degree++;
                } else {
                    is_ptr = true;
                }
            }

            for (size_t j = 0; j < TYPE_COUNT; j++) {
                if (strcmp(type_node->data, types[j]) == 0) {
                    base_data_type = llvm_types[j];
                    break;
                }
            }
        }
    }

    if (base_data_type == NULL) {
        printf("Error: Pointer '%s' could not be declared due to empty base data type\n", var_name);
        return;
    }

    type = base_data_type;
    LLVMTypeRef base_type = NULL;
    for (size_t i = 0; i < pointer_degree; i++) {
        base_type = type;
        type = LLVMPointerType(type, 0);
    }

    if (var_name != NULL) {
        // Allocate variable
        LLVMValueRef pointer = LLVMBuildAlloca(builder, type, var_name);
        //  Add pointer to current scope
        current_scope_pointers[current_scope_pointer_count] = pointer;
        current_scope_pointer_names[current_scope_pointer_count] = var_name;
        current_scope_pointer_types[current_scope_pointer_count] = type;
        current_scope_pointer_base_types[current_scope_pointer_count] = base_type;
        current_scope_pointer_count++;

    } else {
        printf("Error: Variable '%s' could not be declared\n", var_name);
    }
}

void visit_node_pointer_deref(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    const char* pointer_name = node->data;
    LLVMValueRef pointer = NULL;
    size_t idx = 0;
    bool found = false;

    // Check if the pointer is one of the function arguments
    uint32_t arg_count = LLVMCountParams(current_function);
    for (size_t i = 0; i < arg_count; i++) {
        LLVMValueRef arg = LLVMGetParam(current_function, i);
        const char* arg_name = LLVMGetValueName(arg);
        if (strcmp(pointer_name, arg_name) == 0) {
            pointer = arg;
            found = true;
            break;
        }
    }

    if (!found) {
        for (size_t i = 0; i < current_scope_pointer_count; i++) {
            if (strcmp(pointer_name, current_scope_pointer_names[i]) == 0) {
                pointer = current_scope_pointers[i];
                found = true;
                idx = i;
                break;
            }
        }
    }

    if (!found) {
        printf("Error: Pointer '%s' could not be dereferenced\n", pointer_name);
        return;
    }
    Node* expression = node->children[0];
    LLVMValueRef value = visit_node_expression(expression, lexer, module, builder);
    LLVMBuildStore(builder, value, pointer);
    return;
}

void visit_node_function_argument(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

LLVMBasicBlockRef create_if_block(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type, const char* name, LLVMBasicBlockRef merge_block) {
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

    if (LLVMGetBasicBlockTerminator(block) == NULL) {
        LLVMBuildBr(block_builder, merge_block);
    }

    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(block_builder)) == NULL) {
        LLVMBuildBr(block_builder, merge_block);
    }

    LLVMDisposeBuilder(block_builder);
    return block;
}

void visit_node_if_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type) {
    LLVMValueRef condition = NULL;
    LLVMBasicBlockRef if_block = NULL;
    LLVMBasicBlockRef else_block = NULL;
    LLVMBasicBlockRef merge_block = LLVMCreateBasicBlockInContext(LLVMGetModuleContext(module), "ifmrg");

    LLVMBasicBlockRef elif_blocks[100] = {0};
    LLVMBasicBlockRef elif_cond_blocks[100] = {0};
    LLVMValueRef elif_conditions[100] = {0};
    size_t elif_count = 0;

    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_EXPRESSION) {
            condition = visit_node_expression(node->children[i], lexer, module, builder);
        } else if (node->children[i]->type == NODE_BLOCK_STATEMENT) {
            if_block = create_if_block(node->children[i], lexer, module, builder, func, return_type, "if", merge_block);
        } else if (node->children[i]->type == NODE_ELSE_STATEMENT) {
            else_block = create_if_block(node->children[i]->children[0], lexer, module, builder, func, return_type, "else", merge_block);
        } else if (node->children[i]->type == NODE_ELIF_STATEMENT) {
            Node* elif_node = node->children[i];
            for (size_t j = 0; j < elif_node->num_children; j++) {
                if (elif_node->children[j]->type == NODE_EXPRESSION) {
                    elif_conditions[elif_count] = visit_node_expression(elif_node->children[j], lexer, module, builder);
                } else if (elif_node->children[j]->type == NODE_BLOCK_STATEMENT) {
                    elif_blocks[elif_count] = create_if_block(elif_node->children[j], lexer, module, builder, func, return_type, "elif", merge_block);
                }
            }
            elif_count++;
        }
    }

    if (condition != NULL && if_block != NULL) {
        LLVMAppendExistingBasicBlock(func, merge_block);

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
        if (LLVMGetBasicBlockTerminator(if_block) == NULL) {
            LLVMPositionBuilderAtEnd(builder, if_block);
            LLVMBuildBr(builder, merge_block);
        }

        if (elif_count > 0) {
            for (size_t i = 0; i < elif_count; i++) {
                // Check if the elif block has returned
                if (LLVMGetBasicBlockTerminator(elif_blocks[i]) == NULL) {
                    LLVMPositionBuilderAtEnd(builder, elif_blocks[i]);
                    LLVMBuildBr(builder, merge_block);
                }
            }
        }

        if (else_block != merge_block) {
            // Position builder at end of the else block to add the merge block
            if (LLVMGetBasicBlockTerminator(else_block) == NULL) {
                LLVMPositionBuilderAtEnd(builder, else_block);
                LLVMBuildBr(builder, merge_block);
            }
        }
        LLVMPositionBuilderAtEnd(builder, merge_block);
    }

    return;
}

void visit_node_while_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type) {
    LLVMValueRef condition = NULL;
    LLVMBasicBlockRef while_block = LLVMCreateBasicBlockInContext(LLVMGetModuleContext(module), "while");
    LLVMBasicBlockRef merge_block = LLVMCreateBasicBlockInContext(LLVMGetModuleContext(module), "whmerge");

    LLVMBasicBlockRef while_cond_check_block = LLVMCreateBasicBlockInContext(LLVMGetModuleContext(module), "while_cond_check");
    LLVMBuildBr(builder, while_cond_check_block);
    LLVMAppendExistingBasicBlock(func, while_cond_check_block);
    LLVMPositionBuilderAtEnd(builder, while_cond_check_block);

    LLVMBasicBlockRef prev_while_cond_block = while_cond_block;
    LLVMBasicBlockRef prev_while_merge_block = while_merge_block;

    while_cond_block = while_cond_check_block;
    while_merge_block = merge_block;
    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_EXPRESSION) {
            condition = visit_node_expression(node->children[i], lexer, module, builder);
        } else if (node->children[i]->type == NODE_BLOCK_STATEMENT) {
            LLVMBuildCondBr(builder, condition, while_block, merge_block);
            LLVMAppendExistingBasicBlock(func, while_block);
            LLVMPositionBuilderAtEnd(builder, while_block);
            for (size_t j = 0; j < node->children[i]->num_children; j++) {
                visit_node(node->children[i]->children[j], lexer, module, builder);
            }
            LLVMBuildBr(builder, while_cond_check_block);
        }
    }

    if (condition != NULL && while_block != NULL) {
        LLVMAppendExistingBasicBlock(func, merge_block);
        LLVMPositionBuilderAtEnd(builder, merge_block);
    }

    while_cond_block = prev_while_cond_block;
    while_merge_block = prev_while_merge_block;
    return;
}

void visit_node_assignment(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMValueRef value = NULL;
    LLVMValueRef variable = NULL;
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
        LLVMBuildStore(builder, value, variable);
    } else {
        printf("Error: Variable '%s' could not be assigned\n", (char*)node->data);
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

    // Check if pointer is in the current scope
    if (value == NULL) {
        for (int i = 0; i < current_scope_pointer_count; i++) {
            if (strcmp(current_scope_pointer_names[i], identifier) == 0) {
                value = current_scope_pointers[i];
                if (deref) {
                    value = LLVMBuildLoad2(builder, current_scope_pointer_types[i], value, identifier);
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
        } else {
            visit_node(node->children[i], lexer, module, block_builder);
        }
    }
    if (LLVMGetTypeKind(return_type) == LLVMVoidTypeKind) {
        LLVMBuildRetVoid(block_builder);
    } else {
        if (return_value != NULL) {
            if (LLVMGetTypeKind(return_type) == LLVMVoidTypeKind) {
                fprintf(stderr, "Error: Return statement in void function\n");
            } else {
                LLVMBuildRet(block_builder, return_value);
            }
        }
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

LLVMValueRef visit_node_unary_operator(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef value1) {
    const char* op = node->data;

    if (value1 == NULL) {
        printf("Error: Operator '%s' could not be applied\n", op);
        return NULL;
    }

    LLVMTypeRef value1_type = LLVMTypeOf(value1);

    if (strcmp(op, "&") == 0) {
        return value1;
    } else if (strcmp(op, "*") == 0) {
        // Dereference
        // This is a hacky way to dereference a pointer
        LLVMValueRef deref1 = LLVMBuildLoad2(builder, LLVMGetElementType(value1_type), value1, "deref");
        LLVMValueRef deref2 = LLVMBuildLoad2(builder, LLVMGetElementType(LLVMGetElementType(value1_type)), deref1, "deref");
        return deref2;
    }

    if (LLVMGetTypeKind(value1_type) == LLVMIntegerTypeKind) {
        if (strcmp(op, "-") == 0) {
            return LLVMBuildNeg(builder, value1, "negtmp");
        } else if (strcmp(op, "!") == 0) {
            return LLVMBuildNot(builder, value1, "nottmp");
        } else {
            printf("Error: Unsupported operator '%s'\n", op);
        }
    } else if (LLVMGetTypeKind(value1_type) == LLVMFloatTypeKind) {
        if (strcmp(op, "-") == 0) {
            return LLVMBuildFNeg(builder, value1, "negtmp");
        } else {
            printf("Error: Unsupported operator '%s'\n", op);
        }
    } else {
        printf("Error: Unsupported type %s\n", LLVMPrintTypeToString(value1_type));
    }

    return NULL;
}

LLVMValueRef visit_node_binary_operator(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef value1, LLVMValueRef value2) {
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
        } else if (strcmp(op, "/") == 0) {
            return LLVMBuildSDiv(builder, value1, value2, "divtmp");
        } else if (strcmp(op, "%") == 0) {
            return LLVMBuildSRem(builder, value1, value2, "modtmp");
        } else if (strcmp(op, "==") == 0) {
            return LLVMBuildICmp(builder, LLVMIntEQ, value1, value2, "eqtmp");
        } else if (strcmp(op, "!=") == 0) {
            return LLVMBuildICmp(builder, LLVMIntNE, value1, value2, "neqtmp");
        } else if (strcmp(op, "<") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSLT, value1, value2, "lttmp");
        } else if (strcmp(op, ">") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSGT, value1, value2, "gttmp");
        } else if (strcmp(op, "<=") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSLE, value1, value2, "letmp");
        } else if (strcmp(op, ">=") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSGE, value1, value2, "getmp");
        } else if (strcmp(op, "&&") == 0) {
            return LLVMBuildAnd(builder, value1, value2, "andtmp");
        } else if (strcmp(op, "||") == 0) {
            return LLVMBuildOr(builder, value1, value2, "ortmp");
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
            case NODE_ARRAY_ELEMENT:
                lhs = visit_node_array_element(child, lexer, module, builder);
                continue;
            case NODE_IDENTIFIER:
                lhs = visit_node_identifier(child, lexer, module, builder, true);
                continue;
            case NODE_OPERATOR: {
                if (node->num_children < 2) {
                    fprintf(stderr, "Error: Operator '%s' could not be applied\n", (char*)node->data);
                    exit(1);
                    return NULL;
                } else if (node->num_children == 2) {
                    Node* rhs;
                    rhs = node->children[i + 1];
                    LLVMValueRef operand = visit_node(rhs, lexer, module, builder);
                    // Weird hack to get the type of the operand
                    if (strcmp((char*)child->data, "&") == 0 || strcmp((char*)child->data, "*") == 0) {
                        if (rhs->type != NODE_IDENTIFIER) {
                            fprintf(stderr, "Error: Only identifiers can be derefenced. Recieved %s\n", (char*)node->data);
                            exit(1);
                            return NULL;
                        }
                        operand = visit_node_identifier(rhs, lexer, module, builder, false);
                    }
                    lhs = visit_node_unary_operator(child, lexer, module, builder, operand);
                    i++;
                } else if (node->num_children == 3) {
                    Node* rhs;
                    rhs = node->children[i + 1];
                    LLVMValueRef value2 = visit_node(rhs, lexer, module, builder);
                    lhs = visit_node_binary_operator(child, lexer, module, builder, lhs, value2);
                    i++;
                } else {
                    fprintf(stderr, "Error: Operator '%s' could not be applied\n", (char*)node->data);
                    fprintf(stderr, "Error: Node with %lu children\n", node->num_children);
                    exit(1);
                    return NULL;
                }
                continue;
            }
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
            case NODE_STRING_LITERAL:
                lhs = visit_node_string_literal(child, lexer, module, builder);
                continue;
            default:
                printf("Unknown expression node type: %s\n", node_type_to_string(node->type));
                break;
        }
    }
    return lhs;
}

void visit_node_array_declaration(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    const char* array_name = NULL;
    LLVMTypeRef array_types[100] = {0};
    LLVMTypeRef array_element_type = NULL;
    LLVMValueRef array = NULL;
    LLVMTypeRef array_type = NULL;

    size_t num_elements[100] = {0};
    size_t num_dimensions = 0;

    Node* type_node = NULL;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_TYPE) {
            type_node = child;
            for (size_t j = 0; j < TYPE_COUNT; j++) {
                if (strcmp(child->data, types[j]) == 0) {
                    array_element_type = llvm_types[j];
                    break;
                }
            }
        } else if (child->type == NODE_IDENTIFIER) {
            array_name = child->data;
        }
    }

    for (size_t i = 0; i < type_node->num_children; i++) {
        Node* child = type_node->children[i];
        if (child->type == NODE_NUMERIC_LITERAL) {
            num_elements[i] = atoi(child->data);
            num_dimensions++;
        }
    }

    if (array_name != NULL) {
        // reverse the array dimensions
        size_t reversed_num_elements[100] = {0};
        for (size_t i = 0; i < num_dimensions; i++) {
            reversed_num_elements[i] = num_elements[num_dimensions - i - 1];
        }

        // Create the array type
        for (size_t i = 0; i < num_dimensions; i++) {
            if (i == 0) {
                array_type = LLVMArrayType(array_element_type, reversed_num_elements[i]);
            } else {
                array_type = LLVMArrayType(array_type, reversed_num_elements[i]);
            }
        }

        array = LLVMBuildAlloca(builder, array_type, array_name);

        current_scope_arrays[current_scope_array_count] = array;
        current_scope_array_names[current_scope_array_count] = array_name;
        current_scope_array_types[current_scope_array_count] = array_type;
        current_scope_array_element_types[current_scope_array_count] = array_element_type;
        current_scope_array_dims[current_scope_array_count] = num_dimensions;
        current_scope_array_count++;
    } else {
        printf("Error: Array '%s' could not be declared\n", array_name);
    }
}

void visit_node_array_assignment(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    const char* array_name = NULL;
    LLVMValueRef array = NULL;
    LLVMValueRef value = NULL;
    Node* iden = NULL;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            array_name = child->data;
            iden = child;
        } else if (child->type == NODE_EXPRESSION) {
            value = visit_node_expression(child, lexer, module, builder);
        }
    }

    size_t idx = 0;
    bool found = false;
    bool is_pointer = false;
    for (int i = 0; i < current_scope_array_count; i++) {
        if (strcmp(current_scope_array_names[i], array_name) == 0) {
            array = current_scope_arrays[i];
            idx = i;
            found = true;
            break;
        }
    }

    if (!found) {
        for (int i = 0; i < current_scope_pointer_count; i++) {
            if (strcmp(current_scope_pointer_names[i], array_name) == 0) {
                array = current_scope_pointers[i];
                idx = i;
                found = true;
                is_pointer = true;
                break;
            }
        }
    }

    if (array == NULL) {
        printf("Error: Cannot assign to undeclared array '%s'\n", array_name);
        return;
    }

    if (!is_pointer) {
        size_t num_dimensions = current_scope_array_dims[idx];
        LLVMTypeRef array_type = current_scope_array_types[idx];
        LLVMTypeRef array_element_type = current_scope_array_element_types[idx];

        LLVMValueRef zero_index = LLVMConstInt(LLVMInt32Type(), 0, false);

        size_t ind = 0;
        LLVMValueRef indices[2 * num_dimensions];

        for (size_t i = 0; i < iden->num_children; i++) {
            Node* child = iden->children[i];
            if (child->type == NODE_EXPRESSION) {
                indices[2 * ind] = zero_index;
                indices[2 * ind + 1] = visit_node_expression(child, lexer, module, builder);
                ind++;
            }
        }

        LLVMValueRef gep = LLVMBuildGEP2(builder, array_type, array, indices, 2 * num_dimensions, "geptmp");
        LLVMSetIsInBounds(gep, true);
        LLVMBuildStore(builder, value, gep);
    } else {
        LLVMTypeRef array_type = current_scope_pointer_types[idx];
        LLVMTypeRef array_element_type = LLVMGetElementType(array_type);

        size_t num_dimensions = iden->num_children;
        size_t ind = 0;
        LLVMValueRef indices[num_dimensions];

        for (size_t i = 0; i < iden->num_children; i++) {
            Node* child = iden->children[i];
            if (child->type == NODE_EXPRESSION) {
                indices[ind] = visit_node_expression(child, lexer, module, builder);
                printf("Indices[%zu]: %s\n", ind, LLVMPrintValueToString(indices[ind]));
                ind++;
            }
        }

        // Offset the pointer
        LLVMValueRef gep = LLVMBuildGEP2(builder, array_element_type, array, indices, num_dimensions, "geptmp");
        LLVMSetIsInBounds(gep, true);
        LLVMBuildStore(builder, value, gep);
    }
}

LLVMValueRef visit_node_array_element(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    LLVMValueRef array = NULL;
    LLVMValueRef value = NULL;
    const char* array_name = NULL;

    array_name = node->data;
    bool found = false;
    bool is_pointer = false;
    size_t idx = 0;
    for (int i = 0; i < current_scope_array_count; i++) {
        if (strcmp(current_scope_array_names[i], array_name) == 0) {
            array = current_scope_arrays[i];
            idx = i;
            found = true;
            break;
        }
    }

    if (!found) {
        for (int i = 0; i < current_scope_pointer_count; i++) {
            if (strcmp(current_scope_pointer_names[i], array_name) == 0) {
                array = current_scope_pointers[i];
                idx = i;
                found = true;
                is_pointer = true;
                break;
            }
        }
    }

    if (array == NULL) {
        printf("Error: Cannot access undeclared array '%s'\n", array_name);
        return NULL;
    }

    if (!is_pointer) {
        size_t num_dimensions = current_scope_array_dims[idx];
        LLVMTypeRef array_type = current_scope_array_types[idx];
        LLVMTypeRef array_element_type = current_scope_array_element_types[idx];

        LLVMValueRef zero_index = LLVMConstInt(LLVMInt32Type(), 0, false);

        size_t ind = 0;
        LLVMValueRef indices[2 * num_dimensions];

        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_EXPRESSION) {
                indices[2 * ind] = zero_index;
                indices[2 * ind + 1] = visit_node_expression(child, lexer, module, builder);
                ind++;
            }
        }

        LLVMValueRef gep = LLVMBuildGEP2(builder, array_type, array, indices, 2 * num_dimensions, "geptmp");
        LLVMSetIsInBounds(gep, true);
        value = LLVMBuildLoad2(builder, array_element_type, gep, "loadtmp");
        return value;
    } else {
        LLVMTypeRef array_type = current_scope_pointer_types[idx];
        LLVMTypeRef array_element_type = LLVMGetElementType(array_type);

        size_t num_dimensions = node->num_children;
        size_t ind = 0;
        LLVMValueRef indices[num_dimensions];

        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_EXPRESSION) {
                indices[ind] = visit_node_expression(child, lexer, module, builder);
                ind++;
            }
        }

        // Offset the pointer
        LLVMValueRef gep = LLVMBuildGEP2(builder, array_element_type, array, indices, num_dimensions, "geptmp");
        LLVMSetIsInBounds(gep, true);
        value = LLVMBuildLoad2(builder, array_element_type, gep, "loadtmp");
        return value;
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
    bool is_function_vararg = false;
    for (size_t i = 0; i < functionCount; i++) {
        if (strcmp(functionNames[i], function_name) == 0) {
            ret_type = functionReturns[i];
            is_function_vararg = functionIsVararg[i];
        }
    }

    if (ret_type == NULL) {
        printf("Error: Function '%s' not found\n", function_name);
        return NULL;
    }

    // Get function type
    LLVMTypeRef function_type = LLVMFunctionType(ret_type, param_types, param_count, is_function_vararg);

    // Validate against node's number of children - 1 (function name)
    if (!is_function_vararg && node->num_children - 1 != param_count) {
        printf("Error: Incorrect number of arguments for function '%s'\n", function_name);
        return NULL;
    } else if (is_function_vararg && node->num_children - 1 < param_count) {
        printf("Error: Too few arguments for variadic function '%s'\n", function_name);
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

    // Check if the function is void
    LLVMValueRef ret;
    if (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind) {
        return LLVMBuildCall2(builder, function_type, function, args, num_args, "");
    } else {
        ret = LLVMBuildCall2(builder, function_type, function, args, num_args, "calltmp");
    }
    free(param_types);  // free parameter types
    free(args);
    return ret;
}

char* unescape_string(const char* input) {
    size_t len = strlen(input);
    char* output = malloc(len - 1);  // Allocate memory for the new string
    char* p = output;

    for (size_t i = 1; i < len - 1; ++i) {  // Skip the first and last characters (quotes)
        if (input[i] == '\\') {             // If this is an escape character
            switch (input[++i]) {           // Check the next character
                case 'n':
                    *p++ = '\n';
                    break;
                case 't':
                    *p++ = '\t';
                    break;
                case 'r':
                    *p++ = '\r';
                    break;
                case '0':
                    *p++ = '\0';
                    break;
                case '\\':
                    *p++ = '\\';
                    break;
                case '\"':
                    *p++ = '\"';
                    break;
                default:
                    *p++ = input[i];
                    break;  // If it's not a recognized escape sequence, just copy it
            }
        } else {
            *p++ = input[i];  // If it's not an escape character, just copy it
        }
    }

    *p = '\0';  // Null-terminate the new string
    return output;
}

LLVMValueRef visit_node_string_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    const char* value = unescape_string(node->data);
    LLVMValueRef string = LLVMBuildGlobalStringPtr(builder, value, "strtmp");
    return string;
}

void visit_node_elif_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
    return;
}

void visit_node_else_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder) {
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
