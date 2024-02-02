#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include "client.h"

ClientCommand parse_command(char *cmd_str);
void parse_args(int argc, char **argv);
void client_error(char *format, ...);
void client_log(char *format, ...);

#endif