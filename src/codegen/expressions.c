#include <string.h>

#include "codegen.h"
#include "utils/codegen_data.h"

extern const char* types[];
extern const size_t TYPE_COUNT;

extern CodegenData* codegen_data;
extern LLVMTypeRef* llvm_types;

LLVMValueRef visit_node_unary_operator(Node* node, LLVMBuilderRef builder, LLVMValueRef value1) {
    const char* op = node->data;

    if (value1 == NULL) {
        printf("Error: Operator '%s' could not be applied\n", op);
        return NULL;
    }

    LLVMTypeRef value1_type = LLVMTypeOf(value1);

    if (strcmp(op, "&") == 0) {
        return value1;
    } else if (strcmp(op, "*") == 0) {
        // Dereference
        LLVMValueRef deref1 = LLVMBuildLoad2(builder, LLVMGetElementType(value1_type), value1, "deref");
        return deref1;
    }

    if (LLVMGetTypeKind(value1_type) == LLVMIntegerTypeKind) {
        if (strcmp(op, "-") == 0) {
            return LLVMBuildNeg(builder, value1, "negtmp");
        } else if (strcmp(op, "!") == 0) {
            return LLVMBuildNot(builder, value1, "nottmp");
        } else {
            printf("Error: Unsupported operator '%s'\n", op);
        }
    } else if (LLVMGetTypeKind(value1_type) == LLVMFloatTypeKind) {
        if (strcmp(op, "-") == 0) {
            return LLVMBuildFNeg(builder, value1, "negtmp");
        } else {
            printf("Error: Unsupported operator '%s'\n", op);
        }
    } else {
        printf("Error: Unsupported type %s\n", LLVMPrintTypeToString(value1_type));
    }

    return NULL;
}

LLVMValueRef visit_node_binary_operator(Node* node, LLVMBuilderRef builder, LLVMValueRef value1, LLVMValueRef value2) {
    const char* op = node->data;

    if (value1 == NULL || value2 == NULL) {
        printf("Error: Operator '%s' could not be applied\n", op);
        return NULL;
    }

    LLVMTypeRef value1_type = LLVMTypeOf(value1);
    LLVMTypeRef value2_type = LLVMTypeOf(value2);

    if (LLVMGetTypeKind(value1_type) != LLVMGetTypeKind(value2_type)) {
        printf("Error: Operator '%s' could not be applied to different types\n", op);
        return NULL;
    }

    if (LLVMGetTypeKind(value1_type) == LLVMIntegerTypeKind) {
        if (strcmp(op, "+") == 0) {
            return LLVMBuildAdd(builder, value1, value2, "addtmp");
        } else if (strcmp(op, "-") == 0) {
            return LLVMBuildSub(builder, value1, value2, "subtmp");
        } else if (strcmp(op, "*") == 0) {
            return LLVMBuildMul(builder, value1, value2, "multmp");
        } else if (strcmp(op, "/") == 0) {
            return LLVMBuildSDiv(builder, value1, value2, "divtmp");
        } else if (strcmp(op, "%") == 0) {
            return LLVMBuildSRem(builder, value1, value2, "modtmp");
        } else if (strcmp(op, "==") == 0) {
            return LLVMBuildICmp(builder, LLVMIntEQ, value1, value2, "eqtmp");
        } else if (strcmp(op, "!=") == 0) {
            return LLVMBuildICmp(builder, LLVMIntNE, value1, value2, "neqtmp");
        } else if (strcmp(op, "<") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSLT, value1, value2, "lttmp");
        } else if (strcmp(op, ">") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSGT, value1, value2, "gttmp");
        } else if (strcmp(op, "<=") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSLE, value1, value2, "letmp");
        } else if (strcmp(op, ">=") == 0) {
            return LLVMBuildICmp(builder, LLVMIntSGE, value1, value2, "getmp");
        } else if (strcmp(op, "&&") == 0) {
            return LLVMBuildAnd(builder, value1, value2, "andtmp");
        } else if (strcmp(op, "||") == 0) {
            return LLVMBuildOr(builder, value1, value2, "ortmp");
        } else {
            printf("Error: Unsupported operator '%s'\n", op);
        }
    } else if (LLVMGetTypeKind(value1_type) == LLVMFloatTypeKind) {
        if (strcmp(op, "+") == 0) {
            return LLVMBuildFAdd(builder, value1, value2, "addtmp");
        } else if (strcmp(op, "-") == 0) {
            return LLVMBuildFSub(builder, value1, value2, "subtmp");
        } else if (strcmp(op, "*") == 0) {
            return LLVMBuildFMul(builder, value1, value2, "multmp");
        } else {
            printf("Error: Unsupported operator '%s'\n", op);
        }
    } else {
        printf("Error: Unsupported type %s\n", LLVMPrintTypeToString(value1_type));
    }

    return NULL;
}

LLVMValueRef visit_node_expression(Node* node, LLVMBuilderRef builder) {
    LLVMValueRef lhs = NULL;
    for (size_t i = 0; i < node->num_children; i++) {
        Node* child = node->children[i];
        switch (child->type) {
            case NODE_EXPRESSION:
                lhs = visit_node_expression(child, builder);
                continue;
            case NODE_ARRAY_ELEMENT:
                lhs = visit_node_array_element(child, builder);
                continue;
            case NODE_IDENTIFIER:
                lhs = visit_node_identifier(child, builder, true);
                continue;
            case NODE_OPERATOR: {
                if (node->num_children < 2) {
                    fprintf(stderr, "Error: Operator '%s' could not be applied\n", (char*)node->data);
                    exit(1);
                    return NULL;
                } else if (node->num_children == 2) {
                    Node* rhs;
                    rhs = node->children[i + 1];
                    LLVMValueRef operand = visit_node(rhs, builder);
                    // Weird hack to get the type of the operand
                    if (strcmp((char*)child->data, "&") == 0 || strcmp((char*)child->data, "*") == 0) {
                        if (rhs->type != NODE_IDENTIFIER) {
                            fprintf(stderr, "Error: Only identifiers can be derefenced. Recieved %s\n", (char*)node->data);
                            exit(1);
                            return NULL;
                        }
                        operand = visit_node_identifier(rhs, builder, false);
                    }
                    lhs = visit_node_unary_operator(child, builder, operand);
                    i++;
                } else if (node->num_children == 3) {
                    Node* rhs;
                    rhs = node->children[i + 1];
                    LLVMValueRef value2 = visit_node(rhs, builder);
                    lhs = visit_node_binary_operator(child, builder, lhs, value2);
                    i++;
                } else {
                    fprintf(stderr, "Error: Operator '%s' could not be applied\n", (char*)node->data);
                    fprintf(stderr, "Error: Node with %lu children\n", node->num_children);
                    exit(1);
                    return NULL;
                }
                continue;
            }
            case NODE_CALL_EXPRESSION:
                lhs = visit_node_call_expression(child, builder);
                continue;
            case NODE_NUMERIC_LITERAL:
                lhs = visit_node_numeric_literal(child, builder);
                continue;
            case NODE_FLOAT_LITERAL:
                lhs = visit_node_float_literal(child, builder);
                continue;
            case NODE_TRUE_LITERAL:
                lhs = visit_node_true_literal(child, builder);
                continue;
            case NODE_FALSE_LITERAL:
                lhs = visit_node_false_literal(child, builder);
                continue;
            case NODE_NULL_LITERAL:
                lhs = visit_node_null_literal(child, builder);
                continue;
            case NODE_STRING_LITERAL:
                lhs = visit_node_string_literal(child, builder);
                continue;
            default:
                printf("Unknown expression node type: %s\n", node_type_to_string(node->type));
                break;
        }
    }
    return lhs;
}

char* unescape_string(const char* input) {
    size_t len = strlen(input);
    char* output = malloc(len - 1);  // Allocate memory for the new string
    char* p = output;

    for (size_t i = 1; i < len - 1; ++i) {  // Skip the first and last characters (quotes)
        if (input[i] == '\\') {             // If this is an escape character
            switch (input[++i]) {           // Check the next character
                case 'n':
                    *p++ = '\n';
                    break;
                case 't':
                    *p++ = '\t';
                    break;
                case 'r':
                    *p++ = '\r';
                    break;
                case '0':
                    *p++ = '\0';
                    break;
                case '\\':
                    *p++ = '\\';
                    break;
                case '\"':
                    *p++ = '\"';
                    break;
                default:
                    *p++ = input[i];
                    break;  // If it's not a recognized escape sequence, just copy it
            }
        } else {
            *p++ = input[i];  // If it's not an escape character, just copy it
        }
    }

    *p = '\0';  // Null-terminate the new string
    return output;
}

LLVMValueRef visit_node_string_literal(Node* node, LLVMBuilderRef builder) {
    (void)builder;
    const char* value = unescape_string(node->data);
    LLVMValueRef string = LLVMBuildGlobalStringPtr(builder, value, "strtmp");
    return string;
}

LLVMValueRef visit_node_numeric_literal(Node* node, LLVMBuilderRef builder) {
    (void)builder;
    LLVMContextRef ctx = codegen_data->context;
    const char* value_str = node->data;
    float value = strtof(value_str, NULL);
    return LLVMConstInt(LLVMInt32TypeInContext(ctx), value, 0);
}

LLVMValueRef visit_node_float_literal(Node* node, LLVMBuilderRef builder) {
    (void)builder;
    const char* value_str = node->data;
    float value = strtof(value_str, NULL);
    return LLVMConstReal(LLVMFloatType(), value);
}

LLVMValueRef visit_node_true_literal(Node* node, LLVMBuilderRef builder) {
    (void)node;
    (void)builder;
    return LLVMConstInt(LLVMInt1Type(), 1, 0);
}

LLVMValueRef visit_node_false_literal(Node* node, LLVMBuilderRef builder) {
    (void)node;
    (void)builder;
    return LLVMConstInt(LLVMInt1Type(), 0, 0);
}

LLVMValueRef visit_node_null_literal(Node* node, LLVMBuilderRef builder) {
    (void)node;
    (void)builder;
    return LLVMConstPointerNull(LLVMPointerType(LLVMInt8Type(), 0));
}
