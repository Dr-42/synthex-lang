#pragma once

#include <llvm-c/Core.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct CodegenData_Function {
    const char* function_name;
    LLVMTypeRef return_type;
    LLVMTypeRef* parameter_types;
    size_t parameter_count;
    bool is_vararg;
} CodegenData_Function;

typedef struct CodegenData_Variable {
    const char* variable_name;
    LLVMTypeRef variable_type;
    LLVMValueRef variable;
} CodegenData_Variable;

typedef struct CodegenData_Array {
    const char* array_name;
    LLVMTypeRef array_type;
    LLVMTypeRef array_element_type;
    LLVMValueRef array;
    size_t array_dim;
} CodegenData_Array;

typedef struct CodegenData_Pointer {
    const char* pointer_name;
    LLVMTypeRef pointer_type;
    LLVMTypeRef pointer_base_type;
    LLVMValueRef pointer;
} CodegenData_Pointer;

typedef struct CodegenData {
    CodegenData_Function** functions;
    size_t function_count;

    CodegenData_Variable** variables;
    size_t variable_count;

    CodegenData_Array** arrays;
    size_t array_count;

    CodegenData_Pointer** pointers;
    size_t pointer_count;

    LLVMBasicBlockRef while_merge_block;
    LLVMBasicBlockRef while_cond_block;
    LLVMValueRef current_function;

    LLVMModuleRef module;
    LLVMContextRef context;
} CodegenData;

CodegenData* codegen_data_create();
void codegen_data_destroy(CodegenData* data);

void codegen_data_add_function(CodegenData* data, CodegenData_Function* function);
void codegen_data_add_variable(CodegenData* data, CodegenData_Variable* variable);
void codegen_data_add_array(CodegenData* data, CodegenData_Array* array);
void codegen_data_add_pointer(CodegenData* data, CodegenData_Pointer* pointer);

CodegenData_Function* codegen_data_create_function(const char* function_name, LLVMTypeRef return_type, LLVMTypeRef* parameter_types, size_t parameter_count, bool is_vararg);
void codegen_data_function_destroy(CodegenData_Function* function);

CodegenData_Variable* codegen_data_create_variable(const char* variable_name, LLVMTypeRef variable_type);
void codegen_data_variable_destroy(CodegenData_Variable* variable);

CodegenData_Array* codegen_data_create_array(const char* array_name, LLVMTypeRef array_type, LLVMTypeRef array_element_type, size_t array_dim);
void codegen_data_array_destroy(CodegenData_Array* array);

CodegenData_Pointer* codegen_data_create_pointer(const char* pointer_name, LLVMTypeRef pointer_type, LLVMTypeRef pointer_base_type);
void codegen_data_pointer_destroy(CodegenData_Pointer* pointer);

CodegenData_Function* codegen_data_get_function(CodegenData* data, const char* function_name);
CodegenData_Variable* codegen_data_get_variable(CodegenData* data, const char* variable_name);
CodegenData_Array* codegen_data_get_array(CodegenData* data, const char* array_name);
CodegenData_Pointer* codegen_data_get_pointer(CodegenData* data, const char* pointer_name);

void codegen_data_set_while_merge_block(CodegenData* data, LLVMBasicBlockRef while_merge_block);
void codegen_data_set_while_cond_block(CodegenData* data, LLVMBasicBlockRef while_cond_block);
void codegen_data_set_current_function(CodegenData* data, LLVMValueRef current_function);