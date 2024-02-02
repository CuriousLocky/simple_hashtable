#define _GNU_SOURCE
#include <threads.h>
#include <semaphore.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared_mem.h"
#include "protocol.h"
#include "hashtable.h"
#include "worker.h"
#include "server.h"
#include "server_utils.h"

thread_local int id;
thread_local WorkerSlot *slot;

void pack_nonempty(RequestHeader *request, MemChunk value) {
    int response_shm_id = -1;
    char *response_buffer = create_and_map_shm_id(value.len, &response_shm_id);
    request->response_type = SUCCESS;
    request->key_len = value.len;
    request->response_shm_id = response_shm_id;
    memcpy(response_buffer, value.content, value.len);
    free(value.content);
}

void pack_empty(RequestHeader *request, OperationResponseType type) {
    request->response_type = type;
    request->key_len = 0;
    request->response_shm_id = -1;
}

void process_request(int request_shm_id, size_t request_size) {
    printf("shm_id is %d, size is %ld\n", request_shm_id, request_size);
    RequestHeader *request = map_shm_id(request_shm_id, request_size);
    MemChunk key;
    key.len = request->key_len;
    key.content = malloc(key.len);
    memcpy(key.content, request->request_buffer, key.len);
    switch (request->request_type) {
    case READ: {
        MemChunk value = hashtable_search(server_table, key);
        if (value.content == NULL) {
            pack_empty(request, NO_ELEMEMT);
        } else {
            pack_nonempty(request, value);
        }
    } break;
    case INSERT: {
        MemChunk value;
        value.len = request->value_len;
        value.content = malloc(value.len);
        printf("INSERT, key len %ld, value len %ld\n", key.len, value.len);
        memcpy(value.content, request->request_buffer + key.len, value.len);
        hashtable_insert(server_table, key, value);
        server_log("finished insertion");
        pack_empty(request, SUCCESS);
    } break;
    case DELETE: {
        MemChunk value = hashtable_delete(server_table, key);
        if (value.content == NULL) {
            pack_empty(request, NO_ELEMEMT);
        } else {
            pack_nonempty(request, value);
        }
    } break;
    default: {
        server_log("worker %d: undefined request type %d", id, request->request_type);
        pack_empty(request, INTERNAL_FAIL);
    } break;
    }
    sem_post(&request->semaphore);
    munmap(request, request_size);
}

void *worker_main(void *arg) {
    id = (long)arg;
    slot = &request_pool->slots[id];
    server_log("worker %d successfully initiated", id);
    while (true) {
        if (slot->available == false) {
            slot->available = true;
            sem_post(&request_pool->available_slot_sem);
        }
        int wait_error = sem_wait(&slot->semaphore);
        switch (wait_error) {
        case 0: {
            process_request(slot->request_shm_id, slot->request_size);
        } break;
        case EINTR: {
            continue;
        } break;
        default: {
            server_error("worker %d: wait error %d", id, wait_error);
        }
        }
    }
    return NULL;
}

void launch_worker(int id) {
    pthread_create(&worker_pool[id].thread_id, NULL, worker_main, (void *)(long)id);
}
