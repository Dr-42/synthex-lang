#include "utils/codegen_data.h"

#include <stdlib.h>
#include <string.h>

CodegenData* codegen_data_create(LLVMModuleRef module, LLVMContextRef context) {
    CodegenData* data = malloc(sizeof(CodegenData));
    data->functions = NULL;
    data->variables = NULL;
    data->arrays = NULL;
    data->pointers = NULL;
    data->while_merge_block = NULL;
    data->while_cond_block = NULL;
    data->current_function = NULL;

    data->function_count = 0;
    data->variable_count = 0;
    data->array_count = 0;
    data->pointer_count = 0;

    data->module = module;
    data->context = context;
    return data;
}

void codegen_data_destroy(CodegenData* data) {
    for (size_t i = 0; i < data->function_count; i++) {
        codegen_data_function_destroy(data->functions[i]);
    }
    for (size_t i = 0; i < data->variable_count; i++) {
        codegen_data_variable_destroy(data->variables[i]);
    }
    for (size_t i = 0; i < data->array_count; i++) {
        codegen_data_array_destroy(data->arrays[i]);
    }
    for (size_t i = 0; i < data->pointer_count; i++) {
        codegen_data_pointer_destroy(data->pointers[i]);
    }
    free(data->functions);
    free(data->variables);
    free(data->arrays);
    free(data->pointers);
    free(data);
}

void codegen_data_add_function(CodegenData* data, CodegenData_Function* function) {
    if (data->function_count == 0) {
        data->functions = malloc(sizeof(CodegenData_Function*));
    } else {
        data->functions = realloc(data->functions, sizeof(CodegenData_Function*) * (data->function_count + 1));
    }
    data->functions[data->function_count] = function;
    data->function_count++;
}

void codegen_data_add_variable(CodegenData* data, CodegenData_Variable* variable) {
    if (data->variable_count == 0) {
        data->variables = malloc(sizeof(CodegenData_Variable*));
    } else {
        data->variables = realloc(data->variables, sizeof(CodegenData_Variable*) * (data->variable_count + 1));
    }
    data->variables[data->variable_count] = variable;
    data->variable_count++;
}
void codegen_data_add_array(CodegenData* data, CodegenData_Array* array) {
    if (data->array_count == 0) {
        data->arrays = malloc(sizeof(CodegenData_Array*));
    } else {
        data->arrays = realloc(data->arrays, sizeof(CodegenData_Array*) * (data->array_count + 1));
    }
    data->arrays[data->array_count] = array;
    data->array_count++;
}

void codegen_data_add_pointer(CodegenData* data, CodegenData_Pointer* pointer) {
    if (data->pointer_count == 0) {
        data->pointers = malloc(sizeof(CodegenData_Pointer*));
    } else {
        data->pointers = realloc(data->pointers, sizeof(CodegenData_Pointer*) * (data->pointer_count + 1));
    }
    data->pointers[data->pointer_count] = pointer;
    data->pointer_count++;
}

CodegenData_Function* codegen_data_create_function(const char* function_name, LLVMTypeRef return_type, LLVMTypeRef* parameter_types, size_t parameter_count, bool is_vararg) {
    CodegenData_Function* function = malloc(sizeof(CodegenData_Function));
    function->function_name = function_name;
    function->return_type = return_type;
    function->parameter_types = parameter_types;
    function->parameter_count = parameter_count;
    function->is_vararg = is_vararg;
    return function;
}

void codegen_data_function_destroy(CodegenData_Function* function) {
    free(function);
}

CodegenData_Variable* codegen_data_create_variable(const char* variable_name, LLVMTypeRef variable_type) {
    CodegenData_Variable* variable = malloc(sizeof(CodegenData_Variable));
    variable->variable_name = variable_name;
    variable->variable_type = variable_type;
    return variable;
}

void codegen_data_variable_destroy(CodegenData_Variable* variable) {
    free(variable);
}

CodegenData_Array* codegen_data_create_array(const char* array_name, LLVMTypeRef array_type, LLVMTypeRef array_element_type, size_t array_dim) {
    CodegenData_Array* array = malloc(sizeof(CodegenData_Array));
    array->array_name = array_name;
    array->array_type = array_type;
    array->array_element_type = array_element_type;
    array->array_dim = array_dim;
    return array;
}

void codegen_data_array_destroy(CodegenData_Array* array) {
    free(array);
}

CodegenData_Pointer* codegen_data_create_pointer(const char* pointer_name, LLVMTypeRef pointer_type, LLVMTypeRef pointer_base_type) {
    CodegenData_Pointer* pointer = malloc(sizeof(CodegenData_Pointer));
    pointer->pointer_name = pointer_name;
    pointer->pointer_type = pointer_type;
    pointer->pointer_base_type = pointer_base_type;
    return pointer;
}

void codegen_data_pointer_destroy(CodegenData_Pointer* pointer) {
    free(pointer);
}

CodegenData_Function* codegen_data_get_function(CodegenData* data, const char* function_name) {
    for (size_t i = 0; i < data->function_count; i++) {
        if (strcmp(data->functions[i]->function_name, function_name) == 0) {
            return data->functions[i];
        }
    }
    return NULL;
}

CodegenData_Variable* codegen_data_get_variable(CodegenData* data, const char* variable_name) {
    for (size_t i = 0; i < data->variable_count; i++) {
        if (strcmp(data->variables[i]->variable_name, variable_name) == 0) {
            return data->variables[i];
        }
    }
    return NULL;
}

CodegenData_Array* codegen_data_get_array(CodegenData* data, const char* array_name) {
    for (size_t i = 0; i < data->array_count; i++) {
        if (strcmp(data->arrays[i]->array_name, array_name) == 0) {
            return data->arrays[i];
        }
    }
    return NULL;
}

CodegenData_Pointer* codegen_data_get_pointer(CodegenData* data, const char* pointer_name) {
    for (size_t i = 0; i < data->pointer_count; i++) {
        if (strcmp(data->pointers[i]->pointer_name, pointer_name) == 0) {
            return data->pointers[i];
        }
    }
    return NULL;
}

void codegen_data_set_while_merge_block(CodegenData* data, LLVMBasicBlockRef while_merge_block) {
    data->while_merge_block = while_merge_block;
}
void codegen_data_set_while_cond_block(CodegenData* data, LLVMBasicBlockRef while_cond_block) {
    data->while_cond_block = while_cond_block;
}
void codegen_data_set_current_function(CodegenData* data, LLVMValueRef current_function) {
    data->current_function = current_function;
}