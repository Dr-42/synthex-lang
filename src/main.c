#include <stdio.h>
#include <string.h>

#include "lexer.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Error: Could not open file %s\n", argv[1]);
        return 1;
    }

    char file_contents[1000];
    memset(file_contents, 0, sizeof(file_contents));

    char line[256];
    memset(line, 0, sizeof(line));

    while (fgets(line, sizeof(line), file)) {
        strcat(file_contents, line);
    }

    printf("File contents: \n%s\n", file_contents);

    Lexer *lexer = lexer_create(argv[1], file_contents);
    lexer_print_tokens(lexer);
    lexer_destroy(lexer);

    return 0;
}