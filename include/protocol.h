#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <semaphore.h>
#include <stdbool.h>

#define SHM_ID_PREFIX "/simple_hashtable"

typedef enum {
    INSERT = 10,
    READ,
    DELETE,
} OperationRequestType;

typedef enum {
    SUCCESS,
    NO_ELEMEMT,
    INTERNAL_FAIL,
} OperationResponseType;

typedef struct {
    sem_t server_post_sem;
    OperationRequestType request_type;
    size_t key_len;
    size_t value_len;
    OperationResponseType response_type;
    int response_shm_id;
    char buffer[];
} RequestHeader;

typedef struct {
    sem_t client_post_sem;
    char buffer[];
} ResponseHeader;

typedef struct {
    char available;
    sem_t semaphore;
    size_t request_size;
    int request_shm_id;
} WorkerSlot;

typedef struct {
    sem_t available_slot_sem;
    int slot_num;
    WorkerSlot slots[];
} RequestPool;

#endif