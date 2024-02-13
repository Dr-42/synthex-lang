#include "tests.h"

#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "ast.h"
#include "codegen.h"
#include "lexer.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int test_file(char *filename, char* expected_filename);

void test_all() {
    printf("%sRunning all tests%s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);
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
                printf("%sRunning test%s: %s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET,  filename);
                if(test_file(filename, expected_filename) < 0) {
                    fprintf(stderr, "%sERROR:%s Test failed for file: %s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET, filename);
                } else {
                    printf("%sTest passed for file%s: %s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET, filename);
                }
            }
        }
        closedir(dir);
    } else {
        fprintf(stderr, "%sERROR:%s Failed to open tests directory", ANSI_COLOR_RED, ANSI_COLOR_RESET);
    }

    // Cleanup
    system("rm test.ll");
    system("rm t");
}

int test_file(char *filename, char* expected_filename) {
    Lexer *lexer = lexer_create(filename);
    if (lexer == NULL) {
        fprintf(stderr, "%sERROR:%s Failed to create lexer\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
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
        fprintf(stderr, "%sERROR:%s Failed to open pipe\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
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
        fprintf(stderr, "%sERROR:%sProgram exited with non-zero exit code: %d\n", ANSI_COLOR_RED, ANSI_COLOR_RESET, exit_code);
        return -1;
    }

    // Check if the output is correct
    FILE* expected = fopen(expected_filename, "r");
    if (expected == NULL) {
        fprintf(stderr, "%sERROR:%s Failed to open expected file: %s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET, expected_filename);
        return -1;
    }
    char expected_buffer[4096];
    while (!feof(expected)) {
        fgets(expected_buffer, 4096, expected);
    }

    if (strcmp(buffer, expected_buffer) != 0) {
        fprintf(stderr, "%sERROR:%s Output does not match expected output\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        fprintf(stderr, "\tExpected: %s\n", expected_buffer);
        fprintf(stderr, "\tGot: %s\n", buffer);
        return -1;
    }
    return 0;
}
