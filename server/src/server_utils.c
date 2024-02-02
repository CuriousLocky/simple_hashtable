#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>

#include "server.h"
#include "server_utils.h"

// print help message on how to use this program
void print_help(char *path) {
    printf("usage: %s {options}\n", path);
    printf("options:\n");
    printf("\t--verbose/-v: verbose mode\n");
    printf("\t--size/-s {size-of-hashtable}: set the size of the hashtable, "
        "must be larget than zero, %d as default\n", DEFAULT_HASHTABLE_SIZE);
    printf("\t--help/-h: show this help message\n");
}

struct option long_options[] = {
    {"verbose",     optional_argument,  NULL,   'v' },
    {"size",        required_argument,  NULL,   's' },
    {"help",        optional_argument,  NULL,   'h' },
    {0, 0, 0, 0}
};
char *short_options = "vs:h";
// parse arguments and refuse invalid ones
Args parse_args(int argc, char **argv) {
    int c;
    Args args = { DEFAULT_HASHTABLE_SIZE, true };
    while ((c = getopt_long(argc, argv, short_options, long_options, NULL) != -1)) {
        switch (c) {
        case 's': {
            int size = atoi(optarg);
            if (size <= 0) {
                print_help(argv[0]);
                exit(0);
            }
            args.hashtable_size = size;
        } break;
        case 'v': {
            args.verbose = true;
        } break;
        default: {
            print_help(argv[0]);
            exit(0);
        } break;
        }
    }
    return args;
}

// read processor numbers
// XXX: only works on Linux
int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

// print log if verbose flag is set
void server_log(char *format, ...) {
    if (verbose_flag) {
        char *format_newline = malloc(sizeof(char) * (strlen(format) + 1));
        sprintf(format_newline, "%s\n", format);
        va_list args;
        va_start(args, format);
        vprintf(format_newline, args);
        free(format_newline);
        va_end(args);
    }
}

void server_error(char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    vfprintf(stderr, "\n", args);
    va_end(args);
    exit(0);
}