#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>

#include "ast.h"

void ast_to_llvm(AST* ast, Lexer* lexer);

void visit_node(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_program(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_variable_declaration(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_function_declaration(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_function_argument(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_assignment(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_identifier(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_type(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_block_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_return_statement(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_operator(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_expression(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_numeric_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_float_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_call_expression(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_string_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_true_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_false_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
void visit_node_null_literal(Node* node, Lexer* lexer, LLVMModuleRef module, LLVMBuilderRef builder);
