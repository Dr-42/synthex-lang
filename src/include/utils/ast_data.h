#pragma once
#include "lexer.h"

typedef struct Function {
    const char* name;
    DataType return_type;
    const char** arguments;
    DataType* argument_types;
    size_t argument_count;
} Function;

typedef struct Variable {
    const char* name;
    DataType type;
} Variable;

typedef struct Pointer {
    const char* name;
    DataType base_type;
    size_t degree;
} Pointer;

typedef struct Array {
    const char* name;
    DataType base_type;
    size_t dimension;
} Array;

typedef struct ASTData {
    Function* functions;
    size_t function_count;
    Variable* variables;
    size_t variable_count;
    Pointer* pointers;
    size_t pointer_count;
    Array* arrays;
    size_t array_count;
} ASTData;

ASTData* ast_data_create();
void ast_data_destroy(ASTData* ast_data);

void ast_data_add_function(ASTData* ast_data, Function* function);
void ast_data_add_variable(ASTData* ast_data, Variable* variable);
void ast_data_add_pointer(ASTData* ast_data, Pointer* pointer);
void ast_data_add_array(ASTData* ast_data, Array* array);

void ast_data_print(ASTData* ast_data);

Function* function_create(const char* name, DataType return_type, const char** arguments, DataType* argument_types, size_t argument_count);
void function_destroy(Function* function);

Variable* variable_create(const char* name, DataType type);
void variable_destroy(Variable* variable);

Pointer* pointer_create(const char* name, DataType base_type, size_t degree);
void pointer_destroy(Pointer* pointer);

Array* array_create(const char* name, DataType base_type, size_t dimension);
void array_destroy(Array* array);
