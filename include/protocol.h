#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <semaphore.h>
#include <stdbool.h>

typedef enum {
    INSERT,
    READ,
    DELETE,
} OperationRequestType;

typedef enum {
    SUCCESS,
    NO_ELEMEMT,
    INTERNAL_FAIL,
} OperationResponseType;

typedef struct {
    sem_t semaphore;
    OperationRequestType request_type;
    size_t key_len;
    size_t value_len;
    OperationResponseType response_type;
    int response_fd;
    char request_buffer[];
} RequestHeader;

typedef struct {
    char available;
    sem_t semaphore;
    size_t request_size;
    int request_fd;
} WorkerSlot;

typedef struct {
    sem_t available_slot_sem;
    int slot_num;
    WorkerSlot slots[];
} RequestPool;

#endif