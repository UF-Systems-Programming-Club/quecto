#include "error.h"

#include <stdarg.h>
#include <stdio.h>

bool error = false;
char *src;
size_t src_size;

void report_error(int line, int col, const char *fmt, ...) {
    error = true;

    printf("error %d:%d: ", line, col);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}
