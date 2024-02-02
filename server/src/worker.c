#define _GNU_SOURCE
#include <threads.h>
#include <semaphore.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shared_mem.h"
#include "protocol.h"
#include "hashtable.h"
#include "worker.h"
#include "server.h"
#include "server_utils.h"

thread_local int id;
thread_local WorkerSlot *slot;
thread_local size_t response_buffer_size;
thread_local ResponseHeader *response_buffer;
thread_local int response_buffer_shm_id;
thread_local int response_buffer_fd;

void expand_response_buffer(size_t target_size) {
    ftruncate(response_buffer_fd, target_size);
    response_buffer_size = target_size;
    sem_init(&response_buffer->client_post_sem, 0, 1);
}

void pack_nonempty(RequestHeader *request, MemChunk value) {
    size_t response_body_size = sizeof(ResponseHeader) + value.len;
    if (response_body_size > response_buffer_size) {
        expand_response_buffer(response_body_size);
    }
    request->response_type = SUCCESS;
    request->key_len = value.len + sizeof(ResponseHeader);
    request->response_shm_id = response_buffer_shm_id;
    memcpy(response_buffer->buffer, value.content, value.len);
    free(value.content);
}

void pack_empty(RequestHeader *request, OperationResponseType type) {
    request->response_type = type;
    request->key_len = 0;
    request->response_shm_id = -1;
}

void process_request(int request_shm_id, size_t request_size) {
    server_log("worker %d start processing %d: size %ld", id, request_shm_id, request_size);
    int request_fd = -1;
    RequestHeader *request = map_shm_id(request_shm_id, request_size, &request_fd);
    if (request == NULL) {
        server_error("worker %d mapping %d failed: %s", id, request_shm_id, strerror(errno));
    }
    server_log("request mapped at %p", request);
    MemChunk key;
    key.len = request->key_len;
    key.content = malloc(key.len);
    memcpy(key.content, request->buffer, key.len);
    server_log("worker %d loaded request", id);
    bool has_response = false;
    switch (request->request_type) {
    case READ: {
        MemChunk value = hashtable_search(server_table, key);
        if (value.content == NULL) {
            pack_empty(request, NO_ELEMEMT);
        } else {
            pack_nonempty(request, value);
            has_response = true;
        }
    } break;
    case INSERT: {
        MemChunk value;
        value.len = request->value_len;
        value.content = malloc(value.len);
        memcpy(value.content, request->buffer + key.len, value.len);
        hashtable_insert(server_table, key, value);
        pack_empty(request, SUCCESS);
    } break;
    case DELETE: {
        MemChunk value = hashtable_delete(server_table, key);
        if (value.content == NULL) {
            pack_empty(request, NO_ELEMEMT);
        } else {
            pack_nonempty(request, value);
            has_response = true;
        }
    } break;
    default: {
        server_log("worker %d: undefined request type %d", id, request->request_type);
        pack_empty(request, INTERNAL_FAIL);
    } break;
    }
    sem_post(&request->server_post_sem);
    server_log("unmapping %p, %ld", request, page_align(request_size));
    if (munmap(request, page_align(request_size)) != 0) {
        server_error("worker %d: unmapping failed %s", strerror(errno));
    }
    close(request_fd);
    if (has_response) {
        sem_wait(&response_buffer->client_post_sem);
    }
}

void *worker_main(void *arg) {
    id = (long)arg;
    slot = &request_pool->slots[id];
    response_buffer_size = 4096;
    response_buffer = create_and_map_shm_id(response_buffer_size, &response_buffer_shm_id, &response_buffer_fd);
    sem_init(&response_buffer->client_post_sem, 1, 0);

    server_log("worker %d successfully initiated", id);
    while (true) {
        if (slot->available == false) {
            slot->available = true;
            sem_post(&request_pool->available_slot_sem);
            server_log("worker %d ready", id);
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
