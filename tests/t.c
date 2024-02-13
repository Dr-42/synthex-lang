#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Wrapper around printf
void print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

int get_num() {
    int num;
    scanf("%d", &num);
    return num;
}

int *alloc_dyn_arr(int size) {
    int* res = malloc(size * sizeof(int));
    return res;
}
