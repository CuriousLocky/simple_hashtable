#define _GNU_SOURCE
#include <stdarg.h>
#include <stdlib.h>
#include "println.h"

void println_to(FILE *file, char *format, va_list args) {
    char *format_newline;
    asprintf(&format_newline, "%s\n", format);
    vfprintf(file, format_newline, args);
    free(format_newline);
}