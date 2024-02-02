#ifndef PRINTLN_H
#define PRINTLN_H

#include <stdio.h>
#include <stdarg.h>

void println_to(FILE *file, char *format, ...);

#define println_varg_to(file, format) {\
    va_list args;\
    va_start(args, format);\
    println_to(file, format, args);\
    va_end(args);\
}\


#endif