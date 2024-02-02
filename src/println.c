#define _GNU_SOURCE
#include <stdarg.h>
#include <stdlib.h>
#include "println.h"

void println_to(FILE *file, char *format, ...) {
    char *format_newline;
    asprintf(&format_newline, "%s\n", format);
    va_list args;
    va_start(args, format);
    vfprintf(file, format_newline, args);
    va_end(args);
    free(format_newline);
}