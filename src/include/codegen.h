#pragma once
#include <llvm-c/Core.h>

#include "ast.h"

// In core.c
void ast_to_llvm(AST* ast, const char* filename, const char* output, bool dump);
void convert_all_types(LLVMContextRef ctx);

LLVMValueRef visit_node(Node* node, LLVMBuilderRef builder);

// In types.c
void visit_node_program(Node* node, LLVMBuilderRef builder);
void visit_node_variable_declaration(Node* node, LLVMBuilderRef builder);
void visit_node_function_declaration(Node* node, LLVMBuilderRef builder);
void visit_node_pointer_declaration(Node* node, LLVMBuilderRef builder);
void visit_node_pointer_deref(Node* node, LLVMBuilderRef builder);
void visit_node_function_argument(Node* node, LLVMBuilderRef builder);
void visit_node_if_statement(Node* node, LLVMBuilderRef builder);
void visit_node_while_statement(Node* node, LLVMBuilderRef builder);
void visit_node_assignment(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_identifier(Node* node, LLVMBuilderRef builder, bool deref);
void visit_node_type(Node* node, LLVMBuilderRef builder);
void visit_node_block_statement(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_return_statement(Node* node, LLVMBuilderRef builder);
void visit_node_array_declaration(Node* node, LLVMBuilderRef builder);
void visit_node_array_assignment(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_array_element(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_call_expression(Node* node, LLVMBuilderRef builder);

void visit_node_struct_declaration(Node* node);

// In file expressions.c
LLVMValueRef visit_node_unary_operator(Node* node, LLVMBuilderRef builder, LLVMValueRef value1);
LLVMValueRef visit_node_binary_operator(Node* node, LLVMBuilderRef builder, LLVMValueRef value1, LLVMValueRef value2);
LLVMValueRef visit_node_expression(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_numeric_literal(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_float_literal(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_string_literal(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_true_literal(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_false_literal(Node* node, LLVMBuilderRef builder);
LLVMValueRef visit_node_null_literal(Node* node, LLVMBuilderRef builder);
