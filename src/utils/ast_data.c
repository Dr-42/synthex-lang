#include "utils/ast_data.h"

ASTData* ast_data_create() {
    ASTData* ast_data = malloc(sizeof(ASTData));
    ast_data->functions = NULL;
    ast_data->function_count = 0;
    ast_data->variables = NULL;
    ast_data->variable_count = 0;
    ast_data->pointers = NULL;
    ast_data->pointer_count = 0;
    ast_data->arrays = NULL;
    ast_data->array_count = 0;
    return ast_data;
}

void ast_data_destroy(ASTData* ast_data) {
    for (size_t i = 0; i < ast_data->function_count; i++) {
        function_destroy(&ast_data->functions[i]);
    }
    for (size_t i = 0; i < ast_data->variable_count; i++) {
        variable_destroy(&ast_data->variables[i]);
    }
    for (size_t i = 0; i < ast_data->pointer_count; i++) {
        pointer_destroy(&ast_data->pointers[i]);
    }
    for (size_t i = 0; i < ast_data->array_count; i++) {
        array_destroy(&ast_data->arrays[i]);
    }
    free(ast_data->functions);
    free(ast_data->variables);
    free(ast_data->pointers);
    free(ast_data->arrays);
    free(ast_data);
}

void ast_data_add_function(ASTData* ast_data, Function* function) {
    if (ast_data->functions == NULL) {
        ast_data->functions = malloc(sizeof(Function));
    }
    ast_data->functions = realloc(ast_data->functions, sizeof(Function) * (ast_data->function_count + 1));
    ast_data->functions[ast_data->function_count] = *function;
    ast_data->function_count++;
}

void ast_data_add_variable(ASTData* ast_data, Variable* variable) {
    if (ast_data->variables == NULL) {
        ast_data->variables = malloc(sizeof(Variable));
    }
    ast_data->variables = realloc(ast_data->variables, sizeof(Variable) * (ast_data->variable_count + 1));
    ast_data->variables[ast_data->variable_count] = *variable;
    ast_data->variable_count++;
}
void ast_data_add_pointer(ASTData* ast_data, Pointer* pointer) {
    if (ast_data->pointers == NULL) {
        ast_data->pointers = malloc(sizeof(Pointer));
    }
    ast_data->pointers = realloc(ast_data->pointers, sizeof(Pointer) * (ast_data->pointer_count + 1));
    ast_data->pointers[ast_data->pointer_count] = *pointer;
    ast_data->pointer_count++;
}
void ast_data_add_array(ASTData* ast_data, Array* array) {
    if (ast_data->arrays == NULL) {
        ast_data->arrays = malloc(sizeof(Array));
    }
    ast_data->arrays = realloc(ast_data->arrays, sizeof(Array) * (ast_data->array_count + 1));
    ast_data->arrays[ast_data->array_count] = *array;
    ast_data->array_count++;
}

void ast_data_print(ASTData* ast_data) {
    printf("Functions:\n");
    for (size_t i = 0; i < ast_data->function_count; i++) {
        printf("\t%s\n", ast_data->functions[i].name);
    }
    printf("Variables:\n");
    for (size_t i = 0; i < ast_data->variable_count; i++) {
        printf("\t%s\n", ast_data->variables[i].name);
    }
    printf("Pointers:\n");
    for (size_t i = 0; i < ast_data->pointer_count; i++) {
        printf("\t%s\n", ast_data->pointers[i].name);
    }
    printf("Arrays:\n");
    for (size_t i = 0; i < ast_data->array_count; i++) {
        printf("\t%s\n", ast_data->arrays[i].name);
    }
}

Function* function_create(const char* name, DataType return_type, const char** arguments, DataType* argument_types, size_t argument_count) {
    Function* function = malloc(sizeof(Function));
    function->name = name;
    function->return_type = return_type;
    function->arguments = arguments;
    function->argument_types = argument_types;
    function->argument_count = argument_count;
    return function;
}
void function_destroy(Function* function) {
    free(function);
}

Variable* variable_create(const char* name, DataType type) {
    Variable* variable = malloc(sizeof(Variable));
    variable->name = name;
    variable->type = type;
    return variable;
}
void variable_destroy(Variable* variable) {
    free(variable);
}

Pointer* pointer_create(const char* name, DataType base_type, size_t degree) {
    Pointer* pointer = malloc(sizeof(Pointer));
    pointer->name = name;
    pointer->base_type = base_type;
    pointer->degree = degree;
    return pointer;
}
void pointer_destroy(Pointer* pointer) {
    free(pointer);
}

Array* array_create(const char* name, DataType base_type, size_t dimension) {
    Array* array = malloc(sizeof(Array));
    array->name = name;
    array->base_type = base_type;
    array->dimension = dimension;
    return array;
}
void array_destroy(Array* array) {
    free(array);
}
