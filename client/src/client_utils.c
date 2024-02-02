#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>

#include "println.h"
#include "client.h"
#include "client_utils.h"

void parse_command_failed(char *cmd_str) {
    printf("failed to parse command %s\n", cmd_str);
    exit(0);
}

ClientCommand parse_command(char *cmd_str) {
    ClientCommand result_command;
    result_command.value_len = 0;
    char *cmd_str_copy = NULL;
    sscanf(cmd_str, "%m[^\n]", &cmd_str_copy);

    // parse command type
    char *delim = " ";
    char *cmd_type = strtok(cmd_str_copy, delim);

    if (cmd_type == NULL) { parse_command_failed(cmd_str); }
    if (strcmp(cmd_type, "INSERT") == 0) {
        result_command.type = INSERT;
    } else if (strcmp(cmd_type, "DELETE") == 0) {
        result_command.type = DELETE;
    } else if (strcmp(cmd_type, "READ") == 0) {
        result_command.type = READ;
    } else { parse_command_failed(cmd_str); }

    // parse key as char array
    char *key_str = strtok(NULL, delim);
    if (key_str == NULL) { parse_command_failed(cmd_str); }
    sscanf(key_str, "%ms", &result_command.key);
    result_command.key_len = strlen(key_str) + 1;

    // parse value 
    char *value_str = strtok(NULL, delim);
    if (value_str != NULL) {
        sscanf(value_str, "%ms", &result_command.value);
        result_command.value_len = strlen(value_str) + 1;
    } else if (result_command.type == INSERT) {
        // INSERT requires value
        parse_command_failed(cmd_str);
    } else {
        result_command.value = malloc(1);
        *result_command.value = '\0';
    }

    free(cmd_str_copy);
    return result_command;
}

// print help message on how to use this program
void print_help(char *path) {
    printf("usage: %s {options}\n", path);
    printf("options:\n");
    printf("\t--help/-h: show this help message\n");
    printf("\t--mode/-m: determine the running mode of the client.\n"
        "\t\t\"%d\" for interractive (default).\n"
        "\t\t\"%d\" for single command.\n", INTERACTIVE, COMMAND);
    printf("\t--command/-c: command for single command mode.\n"
        "\t\tignored if mode is interractive\n");
}

struct option long_options[] = {
    {"help",        no_argument,  NULL,   'h' },
    {"mode",        required_argument,  NULL,   'm' },
    {"command",     required_argument,  NULL,   'c' },
    {0, 0, 0, 0}
};
char *short_options = "hm:c:";
// parse arguments and refuse invalid ones
void parse_args(int argc, char **argv) {
    int c = 0;
    // Args args = { DEFAULT_HASHTABLE_SIZE, true };
    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (c) {
        case 'm': {
            ClientMode mode = (ClientMode)atoi(optarg);
            switch (mode) {
            case INTERACTIVE: break;
            case COMMAND: break;
            default: {
                printf("unknown mode\n");
                print_help(argv[0]);
                exit(0);
            }
            }
            client_mode = mode;
        } break;
        case 'c': {
            current_command = parse_command(optarg);
        } break;
        default: {
            print_help(argv[0]);
            exit(0);
        } break;
        }
    }
}

void client_log(char *format, ...) {
    println_varg_to(stdout, format);
}

void client_error(char *format, ...) {
    println_varg_to(stderr, format);
    exit(0);
}