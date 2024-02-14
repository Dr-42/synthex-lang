#pragma once
#include "lexer.h"

typedef enum {
    DATA_TYPE_I8 = 0,
    DATA_TYPE_I16,
    DATA_TYPE_I32,
    DATA_TYPE_I64,
    DATA_TYPE_F32,
    DATA_TYPE_F64,
    DATA_TYPE_STR,
    DATA_TYPE_CHR,
    DATA_TYPE_BLN,
    DATA_TYPE_VOID,
    DATA_TYPE_PTR,
    DATA_TYPE_TOTAL,
} BuiltInDataTypes;

typedef struct Function {
    const char* name;
    DataType* return_type;
    const char** arguments;
    DataType** argument_types;
    size_t argument_count;
} Function;

typedef struct Variable {
    const char* name;
    DataType* type;
} Variable;

typedef struct Pointer {
    const char* name;
    DataType* base_type;
    size_t degree;
} Pointer;

typedef struct Array {
    const char* name;
    DataType* base_type;
    size_t dimension;
} Array;

typedef struct Struct {
    const char* name;
    Variable* members;
    size_t member_count;
} Struct;

typedef struct ASTData {
    DataType* data_types;
    size_t data_type_count;
    Function* functions;
    size_t function_count;
    Variable* variables;
    size_t variable_count;
    Pointer* pointers;
    size_t pointer_count;
    Array* arrays;
    size_t array_count;
    Struct* structs;
    size_t struct_count;
} ASTData;

ASTData* ast_data_create();
void ast_data_destroy(ASTData* ast_data);
void ast_data_add_builtin_types(ASTData* ast_data);

void ast_data_add_function(ASTData* ast_data, Function* function);
void ast_data_add_variable(ASTData* ast_data, Variable* variable);
void ast_data_add_pointer(ASTData* ast_data, Pointer* pointer);
void ast_data_add_array(ASTData* ast_data, Array* array);
void ast_data_add_struct(ASTData* ast_data, Struct* strct);

void ast_data_print(ASTData* ast_data);

Function* ast_data_function_create(const char* name, DataType* return_type, const char** arguments, DataType** argument_types, size_t argument_count);
void ast_data_function_destroy(Function* function);

Variable* ast_data_variable_create(const char* name, DataType* type);
void ast_data_variable_destroy(Variable* variable);

Pointer* ast_data_pointer_create(const char* name, DataType* base_type, size_t degree);
void ast_data_pointer_destroy(Pointer* pointer);

Array* ast_data_array_create(const char* name, DataType* base_type, size_t dimension);
void ast_data_array_destroy(Array* array);

Struct* ast_data_struct_create(const char* name);
void ast_data_struct_add_member(Struct* strct, Variable* member);
void ast_data_struct_destroy(Struct* strct);
