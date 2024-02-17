#include "utils/ast_data.h"

#include <stdlib.h>

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
    ast_data->structs = NULL;
    ast_data->struct_count = 0;
    
    ast_data_add_builtin_types(ast_data);
    return ast_data;
}

void ast_data_destroy(ASTData* ast_data) {
    for (size_t i = 0; i < ast_data->function_count; i++) {
        ast_data_function_destroy(ast_data->functions[i]);
    }
    for (size_t i = 0; i < ast_data->variable_count; i++) {
        ast_data_variable_destroy(ast_data->variables[i]);
    }
    for (size_t i = 0; i < ast_data->pointer_count; i++) {
        ast_data_pointer_destroy(ast_data->pointers[i]);
    }
    for (size_t i = 0; i < ast_data->array_count; i++) {
        ast_data_array_destroy(ast_data->arrays[i]);
    }
    for (size_t i = 0; i < ast_data->struct_count; i++) {
        ast_data_struct_destroy(ast_data->structs[i]);
    }
    free(ast_data->functions);
    free(ast_data->variables);
    free(ast_data->pointers);
    free(ast_data->arrays);
    free(ast_data->structs);
    free(ast_data->data_types);
    free(ast_data);
}

void ast_data_add_builtin_types(ASTData* ast_data) {
    ast_data->data_type_count = DATA_TYPE_TOTAL;
    ast_data->data_types = malloc(sizeof(DataType) * ast_data->data_type_count);
    ast_data->data_types[0] = (DataType) {
        .id = DATA_TYPE_I8,
        .name = "i8",
        .builtin = true,
    };
    ast_data->data_types[1] = (DataType) {
        .id = DATA_TYPE_I16,
        .name = "i16",
        .builtin = true,
    };
    ast_data->data_types[2] = (DataType) {
        .id = DATA_TYPE_I32,
        .name = "i32",
        .builtin = true,
    };
    ast_data->data_types[3] = (DataType) {
        .id = DATA_TYPE_I64,
        .name = "i64",
        .builtin = true,
    };
    ast_data->data_types[4] = (DataType) {
        .id = DATA_TYPE_F32,
        .name = "f32",
        .builtin = true,
    };
    ast_data->data_types[5] = (DataType) {
        .id = DATA_TYPE_F64,
        .name = "f64",
        .builtin = true,
    };
    ast_data->data_types[6] = (DataType) {
        .id = DATA_TYPE_STR,
        .name = "str",
        .builtin = true,
    };
    ast_data->data_types[7] = (DataType) {
        .id = DATA_TYPE_CHR,
        .name = "chr",
        .builtin = true,
    };
    ast_data->data_types[8] = (DataType) {
        .id = DATA_TYPE_BLN,
        .name = "bln",
        .builtin = true,
    };
    ast_data->data_types[9] = (DataType) {
        .id = DATA_TYPE_VOID,
        .name = "void",
        .builtin = true,
    };
    ast_data->data_types[10] = (DataType) {
        .id = DATA_TYPE_PTR,
        .name = "ptr",
        .builtin = true,
    };
}

void ast_data_add_function(ASTData* ast_data, Function* function) {
    ast_data->functions = realloc(ast_data->functions, sizeof(Function*) * (ast_data->function_count + 1));
    ast_data->functions[ast_data->function_count] = function;
    ast_data->function_count++;
}

void ast_data_add_variable(ASTData* ast_data, Variable* variable) {
    ast_data->variables = realloc(ast_data->variables, sizeof(Variable*) * (ast_data->variable_count + 1));
    ast_data->variables[ast_data->variable_count] = variable;
    ast_data->variable_count++;
}
void ast_data_add_pointer(ASTData* ast_data, Pointer* pointer) {
    ast_data->pointers = realloc(ast_data->pointers, sizeof(Pointer*) * (ast_data->pointer_count + 1));
    ast_data->pointers[ast_data->pointer_count] = pointer;
    ast_data->pointer_count++;
}
void ast_data_add_array(ASTData* ast_data, Array* array) {
    ast_data->arrays = realloc(ast_data->arrays, sizeof(Array*) * (ast_data->array_count + 1));
    ast_data->arrays[ast_data->array_count] = array;
    ast_data->array_count++;
}

void ast_data_add_struct(ASTData* ast_data, Struct* strct) {
    ast_data->structs = realloc(ast_data->structs, sizeof(Struct*) * (ast_data->struct_count + 1));
    ast_data->structs[ast_data->struct_count] = strct;
    ast_data->struct_count++;

    ast_data->data_type_count++;
    ast_data->data_types = realloc(ast_data->data_types, sizeof(DataType) * ast_data->data_type_count);
    ast_data->data_types[ast_data->data_type_count - 1] = (DataType) {
        .id = ast_data->data_type_count - 1,
        .name = strct->name,
        .builtin = false,
    };
}

void ast_data_print(ASTData* ast_data) {
    printf("Functions:\n");
    for (size_t i = 0; i < ast_data->function_count; i++) {
        printf("\t%s\n", ast_data->functions[i]->name);
    }
    printf("Variables:\n");
    for (size_t i = 0; i < ast_data->variable_count; i++) {
        printf("\t%s\n", ast_data->variables[i]->name);
    }
    printf("Pointers:\n");
    for (size_t i = 0; i < ast_data->pointer_count; i++) {
        printf("\t%s\n", ast_data->pointers[i]->name);
    }
    printf("Arrays:\n");
    for (size_t i = 0; i < ast_data->array_count; i++) {
        printf("\t%s\n", ast_data->arrays[i]->name);
    }
    printf("Structs:\n");
    for (size_t i = 0; i < ast_data->struct_count; i++) {
        printf("\t%s\n", ast_data->structs[i]->name);
    }
}

Function* ast_data_function_create(const char* name, DataType* return_type, const char** arguments, DataType** argument_types, size_t argument_count) {
    Function* function = malloc(sizeof(Function));
    function->name = name;
    function->return_type = return_type;
    function->arguments = arguments;
    function->argument_types = argument_types;
    function->argument_count = argument_count;
    return function;
}
void ast_data_function_destroy(Function* function) {
    free(function);
}

Variable* ast_data_variable_create(const char* name, DataType* type) {
    Variable* variable = malloc(sizeof(Variable));
    variable->name = name;
    variable->type = type;
    return variable;
}
void ast_data_variable_destroy(Variable* variable) {
    free(variable);
}

Pointer* ast_data_pointer_create(const char* name, DataType* base_type, size_t degree) {
    Pointer* pointer = malloc(sizeof(Pointer));
    pointer->name = name;
    pointer->base_type = base_type;
    pointer->degree = degree;
    return pointer;
}
void ast_data_pointer_destroy(Pointer* pointer) {
    free(pointer);
}

Array* ast_data_array_create(const char* name, DataType* base_type, size_t dimension) {
    Array* array = malloc(sizeof(Array));
    array->name = name;
    array->base_type = base_type;
    array->dimension = dimension;
    return array;
}
void ast_data_array_destroy(Array* array) {
    free(array);
}

Struct* ast_data_struct_create(const char* name) {
    Struct* strct = malloc(sizeof(Struct));
    strct->name = name;
    strct->members = NULL;
    strct->member_count = 0;
    return strct;
}

void ast_data_struct_add_member(Struct* strct, Variable* member) {
    if (strct->members == NULL) {
        strct->members = malloc(sizeof(Variable));
    }
    strct->members = realloc(strct->members, sizeof(Variable) * (strct->member_count + 1));
    strct->members[strct->member_count] = *member;
    ast_data_variable_destroy(member);
    strct->member_count++;
}

void ast_data_struct_destroy(Struct* strct) {
    for (size_t i = 0; i < strct->member_count; i++) {
        ast_data_variable_destroy(&strct->members[i]);
    }
    free(strct->members);
    free(strct);
}
