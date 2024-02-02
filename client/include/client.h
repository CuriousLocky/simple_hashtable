#ifndef CLIENT_H
#define CLIENT_H

#include "protocol.h"

typedef enum {
    INTERACTIVE = 1,
    COMMAND,
} ClientMode;

typedef struct {
    OperationRequestType type;
    size_t key_len;
    char *key;
    size_t value_len;
    char *value;
} ClientCommand;

extern ClientMode client_mode;
extern ClientCommand current_command;
extern char *request_pool_path;

#endif