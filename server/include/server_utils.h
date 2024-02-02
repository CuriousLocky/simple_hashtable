#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdbool.h>

typedef struct Args {
    int hashtable_size;
    bool verbose;
} Args;

// parse arguments and refuse invalid ones
Args parse_args(int argc, char **argv);

// parse argument and return hash table size
int get_hashtable_size(int argc, char **argv);

// get the number of cpus in the current machine
int get_cpu_count();

// print log if verbose flag is set
void server_log(char *format, ...);

// print log and exit
void server_error(char *format, ...);

#endif