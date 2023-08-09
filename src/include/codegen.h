#pragma once
#include <llvm-c/Core.h>

#include "ast.h"

void ast_to_llvm(AST* ast, const char* filename);

LLVMValueRef visit_node(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_program(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_variable_declaration(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_function_declaration(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_pointer_declaration(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_pointer_deref(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_function_argument(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_if_statement(Node* node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type);
void visit_node_while_statement(Node* node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type);
void visit_node_assignment(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_identifier(Node* node, LLVMModuleRef module, LLVMBuilderRef builder, bool deref);
void visit_node_type(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_block_statement(Node* node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef func, LLVMTypeRef return_type);
LLVMValueRef visit_node_return_statement(Node* node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMTypeRef return_type);
LLVMValueRef visit_node_unary_operator(Node* node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef value1);
LLVMValueRef visit_node_binary_operator(Node* node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef value1, LLVMValueRef value2);
void visit_node_array_declaration(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_array_assignment(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_array_element(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_expression(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_numeric_literal(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_float_literal(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_call_expression(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_string_literal(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_true_literal(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_false_literal(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef visit_node_null_literal(Node* node, LLVMModuleRef module, LLVMBuilderRef builder);
