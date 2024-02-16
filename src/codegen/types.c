#include <assert.h>
#include <llvm-c/Core.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "utils/ast_data.h"
#include "utils/codegen_data.h"

extern const char* types[];
extern const size_t BUILTIN_TYPE_COUNT;
extern size_t user_type_count;

extern ASTData* ast_data;
extern CodegenData* codegen_data;
extern LLVMTypeRef* llvm_types;

void visit_node_program(Node* node, LLVMBuilderRef builder) {
    for (size_t i = 0; i < node->num_children; i++) {
        visit_node(node->children[i], builder);
    }
}

void visit_node_variable_declaration(Node* node, LLVMBuilderRef builder) {
    LLVMTypeRef type;
    char* var_name = NULL;

    var_name = node->data;
    const char* type_name = node->children[0]->data;
    size_t data_type = get_data_type(type_name, ast_data)->id;
    type = llvm_types[data_type];

    if (var_name != NULL) {
        // Allocate variable
        LLVMValueRef variable = LLVMBuildAlloca(builder, type, var_name);
        //  Add variable to current scope
        CodegenData_Variable* var = codegen_data_create_variable(var_name, variable, type_name, type);
        codegen_data_add_variable(codegen_data, var);
    } else {
        printf("Error: Variable '%s' could not be declared\n", var_name);
    }
}

void visit_node_function_declaration(Node* node, LLVMBuilderRef builder) {
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
            size_t data_type = get_data_type(child->data, ast_data)->id;
            if (data_type == DATA_TYPE_PTR) {
                size_t pointer_degree = 1;
                Node* type_node = child->children[0];
                bool is_ptr = false;
                while (!is_ptr) {
                    if (get_data_type(type_node->data, ast_data)->id == DATA_TYPE_PTR) {
                        type_node = type_node->children[0];
                        pointer_degree++;
                    } else {
                        is_ptr = true;
                    }
                }

                LLVMTypeRef type = NULL;
                size_t data_type = get_data_type(type_node->data, ast_data)->id;
                if (data_type == ast_data->data_type_count) {
                    fprintf(stderr, "Error: Pointer '%s' could not be declared due to empty base data type\n", func_name);
                    return;
                }
                type = llvm_types[data_type];

                for (size_t i = 0; i < pointer_degree; i++) {
                    type = LLVMPointerType(type, 0);
                }
                return_type = type;
            } else {
                return_type = llvm_types[data_type];
            }
            break;
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
                    is_vararg = true;
                    arg_types[arg_index] = LLVMPointerType(LLVMVoidType(), 0);
                    arg_names[arg_index] = child->data;
                    arg_index++;
                    continue;
                }
                Node* type_node = child->children[0];
                if (get_data_type(type_node->data, ast_data)->id == DATA_TYPE_PTR) {
                    LLVMTypeRef base_data_type = NULL;
                    size_t pointer_degree = 0;
                    LLVMTypeRef type;
                    bool is_ptr = false;
                    while (!is_ptr) {
                        if (get_data_type(type_node->data, ast_data)->id == DATA_TYPE_PTR) {
                            type_node = type_node->children[0];
                            pointer_degree++;
                        } else {
                            is_ptr = true;
                        }
                    }

                    size_t data_type = get_data_type(type_node->data, ast_data)->id;
                    if (data_type == ast_data->data_type_count) {
                        fprintf(stderr, "Error: Pointer '%s' could not be declared due to empty base data type\n", func_name);
                        return;
                    }
                    base_data_type = llvm_types[data_type];
                    if (base_data_type == NULL) {
                        fprintf(stderr, "Error: Pointer '%s' could not be declared due to empty base data type\n", func_name);
                        return;
                    }

                    type = base_data_type;
                    LLVMTypeRef base_type;
                    for (size_t i = 0; i < pointer_degree; i++) {
                        base_type = type;
                        type = LLVMPointerType(base_type, 0);
                    }

                    arg_types[arg_index] = type;
                    arg_names[arg_index] = child->data;
                    arg_index++;
                } else {
                    size_t arg_ty = get_data_type(type_node->data, ast_data)->id;
                    arg_types[arg_index] = llvm_types[arg_ty];
                    arg_names[arg_index] = child->data;
                    arg_index++;
                }
            }
        }

        LLVMTypeRef func_type = LLVMFunctionType(return_type, arg_types, arg_count, is_vararg);
        LLVMValueRef func = LLVMAddFunction(codegen_data->module, func_name, func_type);

        LLVMValueRef* args = calloc(arg_count, sizeof(LLVMValueRef));
        for (size_t i = 0; i < arg_count; i++) {
            args[i] = LLVMGetParam(func, i);
            LLVMSetValueName2(args[i], arg_names[i], strlen(arg_names[i]));
        }
        CodegenData_Function* function = codegen_data_create_function(func_name, func, return_type, arg_types, args, arg_count, is_vararg);
        codegen_data_add_function(codegen_data, function);

        codegen_data_reset_scope(codegen_data);
        codegen_data->current_function = function;

        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_BLOCK_STATEMENT) {
                visit_node_block_statement(child, builder);
            }
        }
        free(arg_names);
    }
}

void visit_node_pointer_declaration(Node* node, LLVMBuilderRef builder) {
    LLVMTypeRef type;
    char* var_name = NULL;
    LLVMTypeRef base_data_type = NULL;
    size_t pointer_degree = 1;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            var_name = child->data;
        } else if (child->type == NODE_TYPE) {
            assert(get_data_type(child->data, ast_data)->id == DATA_TYPE_PTR);
            // Find the pointer type. The child of the pointer type node is the type
            // If the child is a ptr type, then the child of the ptr type node is the type and so on
            Node* type_node = child->children[0];
            bool is_ptr = false;
            while (!is_ptr) {
                if (get_data_type(type_node->data, ast_data)->id == DATA_TYPE_PTR) {
                    type_node = type_node->children[0];
                    pointer_degree++;
                } else {
                    is_ptr = true;
                }
            }
            size_t data_type = get_data_type(type_node->data, ast_data)->id;
            if (data_type == ast_data->data_type_count) {
                fprintf(stderr, "Error: Pointer '%s' could not be declared due to empty base data type\n", var_name);
                return;
            }
            base_data_type = llvm_types[data_type];
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
        CodegenData_Pointer* pointer_data = codegen_data_create_pointer(var_name, pointer, type, base_type, pointer_degree);
        codegen_data_add_pointer(codegen_data, pointer_data);

    } else {
        printf("Error: Variable '%s' could not be declared\n", var_name);
    }
}

void visit_node_pointer_deref(Node* node, LLVMBuilderRef builder) {
    const char* pointer_name = node->data;
    LLVMValueRef pointer = NULL;
    bool found = false;

    // Check if the pointer is one of the function arguments
    uint32_t arg_count = codegen_data->current_function->parameter_count;
    for (size_t i = 0; i < arg_count; i++) {
        LLVMValueRef arg = LLVMGetParam(codegen_data->current_function->function, i);
        const char* arg_name = LLVMGetValueName(arg);
        if (strcmp(pointer_name, arg_name) == 0) {
            pointer = arg;
            found = true;
            break;
        }
    }

    CodegenData_Pointer* pointer_data = codegen_data_get_pointer(codegen_data, pointer_name);
    if (!found) {
        if (pointer_data != NULL) {
            pointer = pointer_data->pointer;
            found = true;
        }
    }

    if (!found) {
        printf("Error: Pointer '%s' could not be dereferenced\n", pointer_name);
        return;
    }
    Node* expression = node->children[0];
    LLVMValueRef value = visit_node_expression(expression, builder);
    LLVMBuildStore(builder, value, pointer);
    return;
}

void visit_node_function_argument(Node* node, LLVMBuilderRef builder) {
    (void)node;
    (void)builder;
    return;
}

LLVMBasicBlockRef create_if_block(Node* node, LLVMBuilderRef builder, const char* name, LLVMBasicBlockRef merge_block) {
    (void)builder;
    LLVMContextRef ctx = codegen_data->context;
    LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(ctx, codegen_data->current_function->function, name);
    LLVMBuilderRef block_builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(block_builder, block);

    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_RETURN_STATEMENT) {
            LLVMValueRef return_value = visit_node_return_statement(node->children[i], block_builder);
            LLVMBuildRet(block_builder, return_value);
        } else if (node->children[i]->type == NODE_VARIABLE_DECLARATION) {
            visit_node_variable_declaration(node->children[i], block_builder);
        } else {
            visit_node(node->children[i], block_builder);
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

void visit_node_if_statement(Node* node, LLVMBuilderRef builder) {
    LLVMValueRef condition = NULL;
    LLVMBasicBlockRef if_block = NULL;
    LLVMBasicBlockRef else_block = NULL;
    LLVMBasicBlockRef merge_block = LLVMCreateBasicBlockInContext(codegen_data->context, "ifmrg");

    LLVMBasicBlockRef elif_blocks[100] = {0};
    LLVMBasicBlockRef elif_cond_blocks[100] = {0};
    LLVMValueRef elif_conditions[100] = {0};
    size_t elif_count = 0;

    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_EXPRESSION) {
            condition = visit_node_expression(node->children[i], builder);
        } else if (node->children[i]->type == NODE_BLOCK_STATEMENT) {
            if_block = create_if_block(node->children[i], builder, "if", merge_block);
        } else if (node->children[i]->type == NODE_ELSE_STATEMENT) {
            else_block = create_if_block(node->children[i]->children[0], builder, "else", merge_block);
        } else if (node->children[i]->type == NODE_ELIF_STATEMENT) {
            Node* elif_node = node->children[i];
            for (size_t j = 0; j < elif_node->num_children; j++) {
                if (elif_node->children[j]->type == NODE_EXPRESSION) {
                    elif_conditions[elif_count] = visit_node_expression(elif_node->children[j], builder);
                } else if (elif_node->children[j]->type == NODE_BLOCK_STATEMENT) {
                    elif_blocks[elif_count] = create_if_block(elif_node->children[j], builder, "elif", merge_block);
                }
            }
            elif_count++;
        }
    }

    if (condition != NULL && if_block != NULL) {
        LLVMAppendExistingBasicBlock(codegen_data->current_function->function, merge_block);

        if (else_block == NULL) {
            else_block = merge_block;
        }

        for (size_t i = 0; i < elif_count; i++) {
            elif_cond_blocks[i] = LLVMCreateBasicBlockInContext(codegen_data->context, "elif_cond");
        }

        // Position builder at end of the function block
        LLVMPositionBuilderAtEnd(builder, LLVMGetInsertBlock(builder));
        if (elif_count > 0) {
            for (size_t i = 0; i < elif_count; i++) {
                if (i == 0) {
                    LLVMBuildCondBr(builder, condition, if_block, elif_cond_blocks[i]);
                }
                LLVMAppendExistingBasicBlock(codegen_data->current_function->function, elif_cond_blocks[i]);
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

void visit_node_while_statement(Node* node, LLVMBuilderRef builder) {
    LLVMValueRef condition = NULL;
    LLVMBasicBlockRef while_block = LLVMCreateBasicBlockInContext(codegen_data->context, "while");
    LLVMBasicBlockRef merge_block = LLVMCreateBasicBlockInContext(codegen_data->context, "whmerge");

    LLVMBasicBlockRef while_cond_check_block = LLVMCreateBasicBlockInContext(codegen_data->context, "while_cond_check");
    LLVMBuildBr(builder, while_cond_check_block);
    LLVMAppendExistingBasicBlock(codegen_data->current_function->function, while_cond_check_block);
    LLVMPositionBuilderAtEnd(builder, while_cond_check_block);

    LLVMBasicBlockRef prev_while_cond_block = codegen_data->while_cond_block;
    LLVMBasicBlockRef prev_while_merge_block = codegen_data->while_merge_block;

    codegen_data->while_cond_block = while_cond_check_block;
    codegen_data->while_merge_block = merge_block;
    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_EXPRESSION) {
            condition = visit_node_expression(node->children[i], builder);
        } else if (node->children[i]->type == NODE_BLOCK_STATEMENT) {
            LLVMBuildCondBr(builder, condition, while_block, merge_block);
            LLVMAppendExistingBasicBlock(codegen_data->current_function->function, while_block);
            LLVMPositionBuilderAtEnd(builder, while_block);
            for (size_t j = 0; j < node->children[i]->num_children; j++) {
                visit_node(node->children[i]->children[j], builder);
            }
            LLVMBuildBr(builder, while_cond_check_block);
        }
    }

    if (condition != NULL && while_block != NULL) {
        LLVMAppendExistingBasicBlock(codegen_data->current_function->function, merge_block);
        LLVMPositionBuilderAtEnd(builder, merge_block);
    }

    codegen_data->while_cond_block = prev_while_cond_block;
    codegen_data->while_merge_block = prev_while_merge_block;
    return;
}

void visit_node_assignment(Node* node, LLVMBuilderRef builder) {
    LLVMValueRef value = NULL;
    LLVMValueRef variable = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            variable = visit_node_identifier(child, builder, false);
        } else if (child->type == NODE_EXPRESSION) {
            value = visit_node_expression(child, builder);
        }
    }

    if (variable != NULL && value != NULL) {
        LLVMBuildStore(builder, value, variable);
    } else {
        printf("Error: Variable '%s' could not be assigned\n", (char*)node->data);
    }
}

LLVMValueRef visit_node_identifier(Node* node, LLVMBuilderRef builder, bool deref) {
    const char* identifier = node->data;
    // LLVMBasicBlockRef currentBlock = LLVMGetInsertBlock(builder);
    LLVMValueRef currentFunction = codegen_data->current_function->function;
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
        for (size_t i = 0; i < codegen_data->variable_count; i++) {
            if (strcmp(codegen_data->variables[i]->variable_name, identifier) == 0) {
                value = codegen_data->variables[i]->variable;
                if (deref) {
                    value = LLVMBuildLoad2(builder, codegen_data->variables[i]->variable_type, value, identifier);
                }
                break;
            }
        }
    }

    // Check if pointer is in the current scope
    if (value == NULL) {
        for (size_t i = 0; i < codegen_data->pointer_count; i++) {
            if (strcmp(codegen_data->pointers[i]->pointer_name, identifier) == 0) {
                value = codegen_data->pointers[i]->pointer;
                if (deref) {
                    value = LLVMBuildLoad2(builder, codegen_data->pointers[i]->pointer_type, value, identifier);
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

void visit_node_type(Node* node, LLVMBuilderRef builder) {
    (void)node;
    (void)builder;
    return;
}

void visit_node_block_statement(Node* node, LLVMBuilderRef builder) {
    (void)builder;
    LLVMContextRef ctx = codegen_data->context;
    LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(ctx, codegen_data->current_function->function, "entry");
    LLVMBuilderRef block_builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(block_builder, block);
    LLVMValueRef return_value = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_RETURN_STATEMENT) {
            return_value = visit_node_return_statement(node->children[i], block_builder);
        } else if (node->children[i]->type == NODE_VARIABLE_DECLARATION) {
            visit_node_variable_declaration(node->children[i], block_builder);
        } else {
            visit_node(node->children[i], block_builder);
        }
    }
    if (LLVMGetTypeKind(codegen_data->current_function->return_type) == LLVMVoidTypeKind) {
        LLVMBuildRetVoid(block_builder);
    } else {
        if (return_value != NULL) {
            if (LLVMGetTypeKind(codegen_data->current_function->return_type) == LLVMVoidTypeKind) {
                fprintf(stderr, "Error: Return statement in void function\n");
            } else {
                LLVMBuildRet(block_builder, return_value);
            }
        }
    }
    LLVMDisposeBuilder(block_builder);
}

LLVMValueRef visit_node_return_statement(Node* node, LLVMBuilderRef builder) {
    LLVMValueRef value = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        if (node->children[i]->type == NODE_EXPRESSION) {
            value = visit_node_expression(node->children[i], builder);
        } else if (node->children[i]->type == NODE_IDENTIFIER) {
            value = visit_node_identifier(node->children[i], builder, true);
        }
    }
    return value;
}

void visit_node_array_declaration(Node* node, LLVMBuilderRef builder) {
    const char* array_name = NULL;
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
            size_t data_type = get_data_type(child->data, ast_data)->id;
            array_element_type = llvm_types[data_type];
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

        CodegenData_Array* array_data = codegen_data_create_array(array_name, array, array_type, array_element_type, num_dimensions);
        codegen_data_add_array(codegen_data, array_data);

    } else {
        printf("Error: Array '%s' could not be declared\n", array_name);
    }
}

void visit_node_array_assignment(Node* node, LLVMBuilderRef builder) {
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
            value = visit_node_expression(child, builder);
        }
    }

    bool found = false;
    bool is_pointer = false;
    CodegenData_Array* array_data = codegen_data_get_array(codegen_data, array_name);
    if (array_data != NULL) {
        array = array_data->array;
        found = true;
    }

    CodegenData_Pointer* pointer_data = codegen_data_get_pointer(codegen_data, array_name);
    if (!found) {
        if (pointer_data != NULL) {
            array = pointer_data->pointer;
            found = true;
            is_pointer = true;
        }
    }

    if (array == NULL) {
        printf("Error: Cannot assign to undeclared array '%s'\n", array_name);
        return;
    }

    if (!is_pointer) {
        size_t num_dimensions = array_data->array_dim;
        LLVMTypeRef array_type = array_data->array_type;
        // LLVMTypeRef array_element_type = array_data->array_element_type;

        LLVMValueRef zero_index = LLVMConstInt(LLVMInt32Type(), 0, false);

        size_t ind = 0;
        LLVMValueRef indices[2 * num_dimensions];

        for (size_t i = 0; i < iden->num_children; i++) {
            Node* child = iden->children[i];
            if (child->type == NODE_EXPRESSION) {
                indices[2 * ind] = zero_index;
                indices[2 * ind + 1] = visit_node_expression(child, builder);
                ind++;
            }
        }

        LLVMValueRef gep = LLVMBuildInBoundsGEP2(builder, array_type, array, indices, 2 * num_dimensions, "geptmp");
        LLVMBuildStore(builder, value, gep);
    } else {
        LLVMTypeRef array_type = pointer_data->pointer_type;
        LLVMTypeRef array_element_type = pointer_data->pointer_base_type;
        LLVMTypeRef pointer_type = LLVMPointerType(array_type, 0);

        size_t num_dimensions = iden->num_children;
        size_t ind = 0;
        LLVMValueRef indices[num_dimensions];

        for (size_t i = 0; i < iden->num_children; i++) {
            Node* child = iden->children[i];
            if (child->type == NODE_EXPRESSION) {
                indices[ind] = visit_node_expression(child, builder);
                ind++;
            }
        }

        // Offset the pointer
        LLVMValueRef array_pointer = LLVMBuildLoad2(builder, pointer_type, array, "arrptr");
        LLVMValueRef gep = LLVMBuildInBoundsGEP2(builder, array_element_type, array_pointer, indices, num_dimensions, "geptmp");
        LLVMBuildStore(builder, value, gep);
    }
}

LLVMValueRef visit_node_array_element(Node* node, LLVMBuilderRef builder) {
    LLVMValueRef array = NULL;
    LLVMValueRef value = NULL;
    const char* array_name = NULL;

    array_name = node->data;
    bool found = false;
    bool is_pointer = false;
    CodegenData_Array* array_data = codegen_data_get_array(codegen_data, array_name);
    if (array_data != NULL) {
        array = array_data->array;
        found = true;
    }

    CodegenData_Pointer* pointer_data = codegen_data_get_pointer(codegen_data, array_name);
    if (!found) {
        if (pointer_data != NULL) {
            array = pointer_data->pointer;
            found = true;
            is_pointer = true;
        }
    }

    if (array == NULL) {
        printf("Error: Cannot access undeclared array '%s'\n", array_name);
        return NULL;
    }

    if (!is_pointer) {
        size_t num_dimensions = array_data->array_dim;
        LLVMTypeRef array_type = array_data->array_type;
        LLVMTypeRef array_element_type = array_data->array_element_type;

        LLVMValueRef zero_index = LLVMConstInt(LLVMInt32Type(), 0, false);

        size_t ind = 0;
        LLVMValueRef indices[2 * num_dimensions];

        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_EXPRESSION) {
                indices[2 * ind] = zero_index;
                indices[2 * ind + 1] = visit_node_expression(child, builder);
                ind++;
            }
        }

        LLVMValueRef gep = LLVMBuildInBoundsGEP2(builder, array_type, array, indices, 2 * num_dimensions, "geptmp");
        value = LLVMBuildLoad2(builder, array_element_type, gep, "loadtmp");
        return value;
    } else {
        LLVMTypeRef array_type = pointer_data->pointer_type;
        LLVMTypeRef array_element_type = pointer_data->pointer_base_type;
        LLVMTypeRef pointer_type = LLVMPointerType(array_type, 0);

        size_t num_dimensions = node->num_children;
        size_t ind = 0;
        LLVMValueRef indices[num_dimensions];

        for (size_t i = 0; i < node->num_children; i++) {
            Node* child = node->children[i];
            if (child->type == NODE_EXPRESSION) {
                indices[ind] = visit_node_expression(child, builder);
                ind++;
            }
        }

        // Offset the pointer
        LLVMValueRef array_pointer = LLVMBuildLoad2(builder, pointer_type, array, "arrptr");
        LLVMValueRef gep = LLVMBuildInBoundsGEP2(builder, array_element_type, array_pointer, indices, num_dimensions, "geptmp");
        value = LLVMBuildLoad2(builder, array_element_type, gep, "loadtmp");
        return value;
    }
}

LLVMValueRef visit_node_call_expression(Node* node, LLVMBuilderRef builder) {
    const char* function_name = node->children[0]->data;
    LLVMValueRef function = codegen_data_get_function(codegen_data, function_name)->function;
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
    CodegenData_Function* function_data = codegen_data_get_function(codegen_data, function_name);
    if (function_data != NULL) {
        ret_type = function_data->return_type;
        is_function_vararg = function_data->is_vararg;
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
            args[arg_count] = visit_node_expression(node->children[i], builder);
            // Promote integer types to 32-bit
            // Promote float types to double
            if (is_function_vararg) {
                if (LLVMGetTypeKind(LLVMTypeOf(args[arg_count])) == LLVMIntegerTypeKind) {
                    if (LLVMGetIntTypeWidth(LLVMTypeOf(args[arg_count])) < 32) {
                        args[arg_count] = LLVMBuildIntCast2(builder, args[arg_count], LLVMInt32Type(), true, "intcast");
                    }
                } else if (LLVMGetTypeKind(LLVMTypeOf(args[arg_count])) == LLVMFloatTypeKind) {
                    args[arg_count] = LLVMBuildFPCast(builder, args[arg_count], LLVMDoubleType(), "fpcast");
                }
            }
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

void visit_node_struct_declaration(Node *node) {
    const char* struct_name = node->data;
    LLVMTypeRef struct_type = NULL;
    size_t member_count = 0;
    LLVMTypeRef* member_types = NULL;
    char** member_names = NULL;
    char** member_type_names = NULL;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_STRUCT_MEMBER) {
            member_count++;
        }
    }

    member_types = calloc(member_count, sizeof(LLVMTypeRef));
    member_names = calloc(member_count, sizeof(char*));
    member_type_names = calloc(member_count, sizeof(char*));
    size_t member_index = 0;
    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_STRUCT_MEMBER) {
            Node* member_child = child->children[0];
            if (member_child->type != NODE_TYPE) {
                printf("Error: Struct member '%s' has no type\n", (char*)member_child->data);
                return;
            }
            size_t data_type = get_data_type(member_child->data, ast_data)->id;
            member_types[member_index] = llvm_types[data_type];
            member_names[member_index] = child->data;
            member_type_names[member_index] = member_child->data;
            member_index++;
        }
    }

    struct_type = LLVMStructCreateNamed(codegen_data->context, struct_name);
    LLVMStructSetBody(struct_type, member_types, member_count, false);
    CodegenData_Struct* struct_data = codegen_data_create_struct(struct_name, struct_type, member_types, member_type_names, member_names, member_count);
    codegen_data_add_struct(codegen_data, struct_data);

    // Add to types
    user_type_count++;
    llvm_types = realloc(llvm_types, sizeof(LLVMTypeRef) * (BUILTIN_TYPE_COUNT + user_type_count));
    llvm_types[BUILTIN_TYPE_COUNT + user_type_count - 1] = struct_type;
    return;
}

void visit_node_struct_member_assignment(Node* node, LLVMBuilderRef builder) {
    char* struct_name = NULL;
    char** member_names = NULL;
    size_t member_depth = 0;
    size_t* member_indices = NULL;
    LLVMValueRef strct = NULL;
    LLVMValueRef value = NULL;
    CodegenData_Struct** structs_data = NULL;

    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        if (child->type == NODE_STRUCT_ACCESS) {
            struct_name = child->data;
            Node* member_child = child->children[0];
            if (member_child->type == NODE_STRUCT_MEMBER) {
                while (true) {
                    member_names = realloc(member_names, sizeof(const char*) * member_depth + 1);
                    member_names[member_depth] = member_child->data;
                    member_depth++;
                    if (member_child->num_children != 0) {
                        member_child = member_child->children[0];
                    } else break;
                }
            }
        } else if (child->type == NODE_EXPRESSION) {
            value = visit_node_expression(child, builder);
        }
    }

    structs_data = malloc(sizeof(CodegenData_Struct*) * member_depth);
    member_indices = malloc(sizeof(size_t) * member_depth);
    CodegenData_Variable* variable_data = codegen_data_get_variable(codegen_data, struct_name);

    bool found = false;
    bool is_pointer = false;
    if (variable_data != NULL) {
        for (size_t i = 0; i < member_depth; i++) {
            if (i == 0) {
                strct = variable_data->variable;
                structs_data[i] = codegen_data_get_struct(codegen_data, variable_data->variable_type_name);
                if (structs_data[i] != NULL) found = true;
            } else {
                CodegenData_Struct* struct_data = structs_data[i - 1];
                for (size_t j = 0; j < struct_data->struct_member_count; j++) {
                    char* struct_member_name = struct_data->struct_member_names[j];
                    char* member_name = member_names[i - 1];
                    if (strcmp(struct_member_name, member_name) == 0) {
                        char* member_type = struct_data->struct_member_type_names[j];
                        structs_data[i] = codegen_data_get_struct(codegen_data, member_type);
                        break;
                    }
                }
            }
        }
    }

    CodegenData_Pointer* pointer_data = codegen_data_get_pointer(codegen_data, struct_name);
    if (!found) {
        if (pointer_data != NULL) {
            strct = pointer_data->pointer;
            assert("Not dealing with pointers to structs now");
            found = true;
            is_pointer = true;
        }
    }

    if (strct == NULL) {
        printf("Error: Cannot assign to undeclared struct '%s'\n", struct_name);
        return;
    }

    if (!is_pointer) {
        LLVMValueRef gep = NULL;
        for (size_t i = 0; i < member_depth; i++) {
            size_t num_members = structs_data[i]->struct_member_count;
            LLVMTypeRef struct_type = structs_data[i]->struct_type;

            for (size_t j = 0; j < num_members; j++) {
                if (strcmp(structs_data[i]->struct_member_names[j], member_names[i]) == 0) {
                    member_indices[i] = j;
                    break;
                }
            }

            if (i == 0) {
                gep = LLVMBuildStructGEP2(builder, struct_type, strct, member_indices[i], "strctgeptmp");
            } else {
                gep = LLVMBuildStructGEP2(builder, struct_type, gep, member_indices[i], "strctgeptmp");
            }
        }
        LLVMBuildStore(builder, value, gep);
    } else {
        fprintf(stderr, "Error: Cannot assign to struct pointer yet '%s'\n", struct_name);
        exit(1);
    }

    free(member_names);
    free(member_indices);
    free(structs_data);
}

LLVMValueRef visit_node_struct_access(Node* node, LLVMBuilderRef builder) {
    const char* struct_name = NULL;
    char** member_names = NULL;
    size_t* member_indices = NULL;
    LLVMValueRef strct = NULL;

    size_t member_depth = 0;
    struct_name = node->data;
    while(true) {
        Node* member_child = node->children[0];
        if (member_child->type != NODE_STRUCT_MEMBER) {
            printf("Error: Struct member '%s' has no type\n", (char*)member_child->data);
            exit(EXIT_FAILURE);
        }
        member_names = realloc(member_names, sizeof(const char*) * member_depth + 1);
        member_names[member_depth] = member_child->data;
        member_depth++;
        if (member_child->num_children != 0) {
            node = member_child;
        } else break;
    }

    CodegenData_Variable* variable_data = codegen_data_get_variable(codegen_data, struct_name);
    bool found = false;
    bool is_pointer = false;

    CodegenData_Struct** structs_data = NULL;
    structs_data = malloc(sizeof(CodegenData_Struct*) * member_depth);
    member_indices = malloc(sizeof(size_t) * member_depth);


    if (variable_data != NULL) {
        for (size_t i = 0; i < member_depth; i++) {
            if (i == 0) {
                strct = variable_data->variable;
                structs_data[i] = codegen_data_get_struct(codegen_data, variable_data->variable_type_name);
                if (strct != NULL) found = true;
            } else {
                CodegenData_Struct* struct_data = structs_data[i - 1];
                for (size_t j = 0; j < struct_data->struct_member_count; j++) {
                    char* struct_member_name = struct_data->struct_member_names[j];
                    char* member_name = member_names[i - 1];
                    if (strcmp(struct_member_name, member_name) == 0) {
                        char* member_type = struct_data->struct_member_type_names[j];
                        structs_data[i] = codegen_data_get_struct(codegen_data, member_type);
                        break;
                    }
                }
            }
        }
    }

    CodegenData_Pointer* pointer_data = codegen_data_get_pointer(codegen_data, struct_name);
    if (!found) {
        if (pointer_data != NULL) {
            strct = pointer_data->pointer;
            assert("Not dealing with pointers to structs now");
            found = true;
            is_pointer = true;
        }
    }

    if (strct == NULL) {
        printf("Error: Cannot access undeclared struct '%s'\n", struct_name);
        return NULL;
    }

    if (!is_pointer) {
        LLVMTypeRef value_type = NULL;
        LLVMValueRef gep = NULL;
        for (size_t i = 0; i < member_depth; i++) {
            size_t num_members = structs_data[i]->struct_member_count;
            LLVMTypeRef struct_type = structs_data[i]->struct_type;

            for (size_t j = 0; j < num_members; j++) {
                if (strcmp(structs_data[i]->struct_member_names[j], member_names[i]) == 0) {
                    member_indices[i] = j;
                    break;
                }
            }

            if (i == 0) {
                gep = LLVMBuildStructGEP2(builder, struct_type, strct, member_indices[i], "strctgeptmp");
            } else {
                gep = LLVMBuildStructGEP2(builder, struct_type, gep, member_indices[i], "strctgeptmp");
            }
            value_type = structs_data[i]->struct_member_types[member_indices[i]];
        }
        LLVMValueRef value = LLVMBuildLoad2(builder, value_type, gep, "loadtmp");
        return value;
    } else {
        fprintf(stderr, "Error: Cannot access to struct pointer yet '%s'\n", struct_name);
        exit(1);
    }

    free(member_names);
    free(member_indices);
    free(structs_data);
}
