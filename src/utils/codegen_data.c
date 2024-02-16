#include "utils/codegen_data.h"

#include <stdlib.h>
#include <string.h>

CodegenData* codegen_data_create(LLVMModuleRef module, LLVMContextRef context) {
    CodegenData* data = malloc(sizeof(CodegenData));
    data->functions = NULL;
    data->variables = NULL;
    data->arrays = NULL;
    data->pointers = NULL;
    data->structs = NULL;
    data->while_merge_block = NULL;
    data->while_cond_block = NULL;
    data->current_function = NULL;

    data->function_count = 0;
    data->variable_count = 0;
    data->array_count = 0;
    data->pointer_count = 0;
    data->struct_count = 0;

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
    for (size_t i = 0; i < data->struct_count; i++) {
        codegen_data_struct_destroy(data->structs[i]);
    }
    free(data->functions);
    free(data->variables);
    free(data->arrays);
    free(data->pointers);
    free(data);
}

void codegen_data_add_function(CodegenData* data, CodegenData_Function* function) {
    data->functions = realloc(data->functions, sizeof(CodegenData_Function*) * (data->function_count + 1));
    data->functions[data->function_count] = function;
    data->function_count++;
}

void codegen_data_add_variable(CodegenData* data, CodegenData_Variable* variable) {
    data->variables = realloc(data->variables, sizeof(CodegenData_Variable*) * (data->variable_count + 1));
    data->variables[data->variable_count] = variable;
    data->variable_count++;
}
void codegen_data_add_array(CodegenData* data, CodegenData_Array* array) {
    data->arrays = realloc(data->arrays, sizeof(CodegenData_Array*) * (data->array_count + 1));
    data->arrays[data->array_count] = array;
    data->array_count++;
}

void codegen_data_add_pointer(CodegenData* data, CodegenData_Pointer* pointer) {
    data->pointers = realloc(data->pointers, sizeof(CodegenData_Pointer*) * (data->pointer_count + 1));
    data->pointers[data->pointer_count] = pointer;
    data->pointer_count++;
}

void codegen_data_add_struct(CodegenData* data, CodegenData_Struct* strukt) {
    data->structs = realloc(data->structs, sizeof(CodegenData_Struct*) * (data->struct_count + 1));
    data->structs[data->struct_count] = strukt;
    data->struct_count++;
}

CodegenData_Function* codegen_data_create_function(const char* function_name, LLVMValueRef function, LLVMTypeRef return_type, LLVMTypeRef* parameter_types, LLVMValueRef* parameters, size_t parameter_count, bool is_vararg) {
    CodegenData_Function* function_data = malloc(sizeof(CodegenData_Function));
    function_data->function_name = function_name;
    function_data->function = function;
    function_data->return_type = return_type;
    function_data->parameter_types = parameter_types;
    function_data->parameter_count = parameter_count;
    function_data->parameters = parameters;
    function_data->is_vararg = is_vararg;
    return function_data;
}

void codegen_data_function_destroy(CodegenData_Function* function) {
    free(function);
}

CodegenData_Variable* codegen_data_create_variable(const char* variable_name, LLVMValueRef variable, const char* variable_type_name, LLVMTypeRef variable_type) {
    CodegenData_Variable* variable_data = malloc(sizeof(CodegenData_Variable));
    variable_data->variable_name = variable_name;
    variable_data->variable = variable;
    variable_data->variable_type = variable_type;
    variable_data->variable_type_name = variable_type_name;
    return variable_data;
}

void codegen_data_variable_destroy(CodegenData_Variable* variable) {
    free(variable);
}

CodegenData_Array* codegen_data_create_array(const char* array_name, LLVMValueRef array, LLVMTypeRef array_type, LLVMTypeRef array_element_type, size_t array_dim) {
    CodegenData_Array* array_data = malloc(sizeof(CodegenData_Array));
    array_data->array_name = array_name;
    array_data->array = array;
    array_data->array_type = array_type;
    array_data->array_element_type = array_element_type;
    array_data->array_dim = array_dim;
    return array_data;
}

void codegen_data_array_destroy(CodegenData_Array* array) {
    free(array);
}

CodegenData_Pointer* codegen_data_create_pointer(const char* pointer_name, LLVMValueRef pointer, LLVMTypeRef pointer_type, LLVMTypeRef pointer_base_type, size_t pointer_degree) {
    CodegenData_Pointer* pointer_data = malloc(sizeof(CodegenData_Pointer));
    pointer_data->pointer_name = pointer_name;
    pointer_data->pointer = pointer;
    pointer_data->pointer_type = pointer_type;
    pointer_data->pointer_base_type = pointer_base_type;
    pointer_data->pointer_degree = pointer_degree;
    return pointer_data;
}

void codegen_data_pointer_destroy(CodegenData_Pointer* pointer) {
    free(pointer);
}

CodegenData_Struct* codegen_data_create_struct(const char* struct_name, LLVMTypeRef struct_type, LLVMTypeRef* struct_member_types, char** member_type_names, char** struct_member_names, size_t struct_member_count) {
    CodegenData_Struct* strukt = malloc(sizeof(CodegenData_Struct));
    strukt->struct_name = struct_name;
    strukt->struct_type = struct_type;
    strukt->struct_member_types = struct_member_types;
    strukt->struct_member_type_names = member_type_names;
    strukt->struct_member_names = struct_member_names;
    strukt->struct_member_count = struct_member_count;
    return strukt;
}

void codegen_data_struct_destroy(CodegenData_Struct* strukt) {
    free(strukt->struct_member_type_names);
    free(strukt->struct_member_names);
    free(strukt);
}

void codegen_data_reset_scope(CodegenData* data) {
    for (size_t i = 0; i < data->variable_count; i++) {
        codegen_data_variable_destroy(data->variables[i]);
    }
    for (size_t i = 0; i < data->array_count; i++) {
        codegen_data_array_destroy(data->arrays[i]);
    }
    for (size_t i = 0; i < data->pointer_count; i++) {
        codegen_data_pointer_destroy(data->pointers[i]);
    }
    data->variable_count = 0;
    data->array_count = 0;
    data->pointer_count = 0;
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

CodegenData_Struct* codegen_data_get_struct(CodegenData* data, const char* struct_name) {
    for (size_t i = 0; i < data->struct_count; i++) {
        if (strcmp(data->structs[i]->struct_name, struct_name) == 0) {
            return data->structs[i];
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
void codegen_data_set_current_function(CodegenData* data, CodegenData_Function* current_function) {
    data->current_function = current_function;
}
