#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <string.h>

#include "codegen.h"
#include "node.h"
#include "utils/codegen_data.h"

extern const char* types[];
extern const size_t BUILTIN_TYPE_COUNT;
size_t user_type_count = 0;

CodegenData* codegen_data;
LLVMTypeRef* llvm_types;

void convert_all_types(LLVMContextRef ctx) {
    llvm_types = calloc(BUILTIN_TYPE_COUNT - 1, sizeof(LLVMTypeRef));
    llvm_types[DATA_TYPE_I8] = LLVMInt8TypeInContext(ctx);
    llvm_types[DATA_TYPE_I16] = LLVMInt16TypeInContext(ctx);
    llvm_types[DATA_TYPE_I32] = LLVMInt32TypeInContext(ctx);
    llvm_types[DATA_TYPE_I64] = LLVMInt64TypeInContext(ctx);
    llvm_types[DATA_TYPE_F32] = LLVMFloatTypeInContext(ctx);
    llvm_types[DATA_TYPE_F64] = LLVMDoubleTypeInContext(ctx);
    llvm_types[DATA_TYPE_STR] = LLVMPointerType(LLVMInt8Type(), 0);
    llvm_types[DATA_TYPE_CHR] = LLVMInt8TypeInContext(ctx);
    llvm_types[DATA_TYPE_BLN] = LLVMInt1TypeInContext(ctx);
    llvm_types[DATA_TYPE_VOID] = LLVMVoidTypeInContext(ctx);
    llvm_types[DATA_TYPE_PTR] = LLVMPointerType(LLVMInt8Type(), 0);
}

void ast_to_llvm(AST* ast, const char* filename, const char* output, bool dump) {
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext(filename, ctx);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

    codegen_data = codegen_data_create(module, ctx);

    convert_all_types(ctx);

    visit_node(ast->root, builder);

    char* error = NULL;
    if (dump) LLVMDumpModule(module);
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    // set target triple for module
    char* target = LLVMGetDefaultTargetTriple();
    LLVMSetTarget(module, target);
    LLVMDisposeMessage(target);

    // Assuming you have an LLVM module `module` and a function named "add" in it
    LLVMExecutionEngineRef engine;
    error = NULL;
    LLVMCreateExecutionEngineForModule(&engine, module, &error);

    LLVMPrintModuleToFile(module, output, &error);
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

LLVMValueRef visit_node(Node* node, LLVMBuilderRef builder) {
    switch (node->type) {
        case NODE_PROGRAM:
            visit_node_program(node, builder);
            break;
        case NODE_VARIABLE_DECLARATION:
            visit_node_variable_declaration(node, builder);
            break;
        case NODE_FUNCTION_DECLARATION:
            visit_node_function_declaration(node, builder);
            break;
        case NODE_POINTER_DECLARATION:
            visit_node_pointer_declaration(node, builder);
            break;
        case NODE_POINTER_DEREF:
            visit_node_pointer_deref(node, builder);
        case NODE_FUNCTION_ARGUMENT:
            visit_node_function_argument(node, builder);
            break;
        case NODE_ASSIGNMENT:
            visit_node_assignment(node, builder);
            break;
        case NODE_IDENTIFIER:
            return visit_node_identifier(node, builder, true);
            break;
        case NODE_TYPE:
            visit_node_type(node, builder);
            break;
        case NODE_BLOCK_STATEMENT:
            break;
        case NODE_RETURN_STATEMENT:
            break;
        case NODE_OPERATOR:
            break;
        case NODE_EXPRESSION:
            return visit_node_expression(node, builder);
            break;
        case NODE_ARRAY_DECLARATION:
            visit_node_array_declaration(node, builder);
            break;
        case NODE_ARRAY_ASSIGNMENT:
            visit_node_array_assignment(node, builder);
            break;
        case NODE_ARRAY_ELEMENT:
            return visit_node_array_element(node, builder);
            break;
        case NODE_STRUCT_DECLARATION:
            visit_node_struct_declaration(node);
            break;
        case NODE_STRUCT_MEMBER_ASSIGNMENT:
            visit_node_struct_member_assignment(node, builder);
            break;
        case NODE_IF_STATEMENT:
            visit_node_if_statement(node, builder);
            break;
        case NODE_WHILE_STATEMENT:
            visit_node_while_statement(node, builder);
            break;
        case NODE_NUMERIC_LITERAL:
            return visit_node_numeric_literal(node, builder);
            break;
        case NODE_FLOAT_LITERAL:
            return visit_node_float_literal(node, builder);
            break;
        case NODE_CALL_EXPRESSION:
            return visit_node_call_expression(node, builder);
            break;
        case NODE_STRING_LITERAL:
            visit_node_string_literal(node, builder);
            break;
        case NODE_TRUE_LITERAL:
            return visit_node_true_literal(node, builder);
            break;
        case NODE_FALSE_LITERAL:
            return visit_node_false_literal(node, builder);
            break;
        case NODE_NULL_LITERAL:
            return visit_node_null_literal(node, builder);
            break;
        case NODE_BRK_STATEMENT:
            if (codegen_data->while_merge_block != NULL && codegen_data->while_cond_block != NULL) {
                LLVMBuildBr(builder, codegen_data->while_merge_block);
            } else {
                fprintf(stderr, "Error: Break statement outside of loop\n");
            }
            break;
        case NODE_CONT_STATEMENT:
            if (codegen_data->while_cond_block != NULL && codegen_data->while_merge_block != NULL) {
                LLVMBuildBr(builder, codegen_data->while_cond_block);
            } else {
                fprintf(stderr, "Error: Continue statement outside of loop\n");
            }
            break;
        case NODE_COMMENT:
            break;
        case NODE_DOC_COMMENT:
            break;
        default:
            printf("Unknown node type: %s\n", node_type_to_string(node->type));
            break;
    }

    return NULL;
}
