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
thread_local int response_shm_id;
thread_local char *response_buffer;
thread_local size_t response_buffer_size;

#define PAGE_SIZE 4096
static inline size_t page_align(size_t base) {
    return PAGE_SIZE + (base & ~(PAGE_SIZE - 1));
}

void expand_response_buffer(size_t target_size) {
    target_size = page_align(target_size);
    response_buffer = mremap(response_buffer, response_buffer_size, target_size, MREMAP_MAYMOVE);
    response_buffer_size = target_size;
}

void pack_success(RequestHeader *request, MemChunk value) {
    request->response_type = SUCCESS;
    request->key_len = value.len;
    request->response_shm_id = response_shm_id;
    if (response_buffer_size < value.len) {
        expand_response_buffer(value.len);
    }
    if (value.content != NULL) {
        memcpy(response_buffer, value.content, value.len);
        free(value.content);
    }
}

void pack_empty(RequestHeader *request, OperationResponseType type) {
    request->response_type = type;
    request->key_len = 0;
    request->response_shm_id = response_shm_id;
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
            pack_success(request, value);
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
        pack_success(request, value);
    } break;
    default: {
        server_log("worker %d: undefined request type %d", id, request->request_type);
        pack_empty(request, INTERNAL_FAIL);
    } break;
    }
    sem_post(&request->semaphore);
}

void *worker_main(void *arg) {
    id = (long)arg;
    slot = &request_pool->slots[id];
    response_buffer_size = PAGE_SIZE;
    response_buffer = create_and_map_shm_id(response_buffer_size, &response_shm_id);
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
