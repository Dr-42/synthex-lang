#include "tests.h"

#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "ast.h"
#include "codegen.h"
#include "lexer.h"

int test_file(char *filename, char* expected_filename);

void test_all() {
    // Get all files in the tests directory
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("tests/cases")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                char filename[256];
                char expected_filename[256];
                snprintf(filename, sizeof(filename), "tests/cases/%s", ent->d_name);
                snprintf(expected_filename, sizeof(expected_filename), "tests/controls/%s", ent->d_name);
                // Replace .syn with .txt
                char *dot = strrchr(expected_filename, '.');
                if (dot != NULL) {
                    strcpy(dot, ".txt");
                }
                printf("Running test: %s\n", filename);
                if(test_file(filename, expected_filename) < 0) {
                    fprintf(stderr, "Test failed for file: %s\n", filename);
                } else {
                    printf("Test passed for file: %s\n", filename);
                }
            }
        }
        closedir(dir);
    } else {
        fprintf(stderr, "Failed to open tests directory");
    }
}

int test_file(char *filename, char* expected_filename) {
    Lexer *lexer = lexer_create(filename);
    if (lexer == NULL) {
        printf("Failed to create lexer\n");
        return -1;
    }
    AST *ast = ast_create();
    ast_build(ast, lexer);
    ast_to_llvm(ast, lexer->filename, "test.ll", false);
    ast_destroy(ast);
    lexer_destroy(lexer);
    
    system("clang test.ll tests/t.c -o t");

    // Create a pipe to read the output of the program
    FILE *pipe = popen("./t", "r");
    if (!pipe) {
        fprintf(stderr, "Failed to open pipe\n");
        return -1;
    }
    char buffer[4096];
    while (!feof(pipe)) {
        fgets(buffer, 4096, pipe);
    }
    pclose(pipe);

    // Get the exit code of the program
    int exit_code = WEXITSTATUS(system("./t >> /dev/null 2>&1"));
    if (exit_code != 0) {
        fprintf(stderr, "Program exited with non-zero exit code: %d\n", exit_code);
        return -1;
    }

    // Check if the output is correct
    FILE* expected = fopen(expected_filename, "r");
    if (expected == NULL) {
        fprintf(stderr, "Failed to open expected file: %s\n", expected_filename);
        return -1;
    }
    char expected_buffer[4096];
    while (!feof(expected)) {
        fgets(expected_buffer, 4096, expected);
    }

    if (strcmp(buffer, expected_buffer) != 0) {
        fprintf(stderr, "Output does not match expected output\n");
        fprintf(stderr, "Expected: %s\n", expected_buffer);
        fprintf(stderr, "Got: %s\n", buffer);
        return -1;
    }
    return 0;
}
