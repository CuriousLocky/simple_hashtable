#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>

#include "server.h"
#include "server_utils.h"
#include "println.h"

// print help message on how to use this program
void print_help(char *path) {
    printf("usage: %s {options}\n", path);
    printf("options:\n");
    printf("\t--verbose/-v: verbose mode\n");
    printf("\t--size/-s {size-of-hashtable}: set the size of the hashtable, "
        "must be larget than zero, %d as default\n", DEFAULT_HASHTABLE_SIZE);
    printf("\t--worker/-w {number-of-workers}: set the number of workers, "
        "must be larger than zero, %d as default\n", DEFAULT_WORKER_NUM);
    printf("\t--help/-h: show this help message\n");
}

struct option long_options[] = {
    {"verbose",     no_argument,  NULL,   'v' },
    {"size",        required_argument,  NULL,   's' },
    {"worker",      required_argument,  NULL,   'w'},
    {"help",        no_argument,  NULL,   'h' },
    {0, 0, 0, 0}
};
char *short_options = "vs:w:h";
// parse arguments and refuse invalid ones
void parse_args(int argc, char **argv) {
    int c;
    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (c) {
        case 's': {
            int size = atoi(optarg);
            if (size <= 0) {
                print_help(argv[0]);
                exit(0);
            }
            hashtable_size = size;
        } break;
        case 'v': {
            verbose_flag = true;
        } break;
        case 'w': {
            int num = atoi(optarg);
            if (num <= 0) {
                print_help(argv[0]);
                exit(0);
            }
            worker_num = num;
        } break;
        default: {
            print_help(argv[0]);
            exit(0);
        } break;
        }
    }
}

// read processor numbers
// XXX: only works on Linux
int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

// print log if verbose flag is set
void server_log(char *format, ...) {
    if (verbose_flag) {
        println_varg_to(stdout, format);
    }
}

void server_error(char *format, ...) {
    println_varg_to(stderr, format);
    exit(0);
}