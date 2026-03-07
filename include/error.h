#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>
#include <stdbool.h>

extern bool error;

// TODO: perform error recovery in parsing and only output a certain number of errors
// instead of flooding the screen (like C and C++)
extern int error_count;

// TODO: print out source file line with indicator of error in report_error
extern char *src;
extern size_t src_size;

void report_error(int line, int col, const char *fmt, ...);

#endif
