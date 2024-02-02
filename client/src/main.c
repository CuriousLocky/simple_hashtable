#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <errno.h>
#include <signal.h>

#include "shared_mem.h"
#include "client.h"
#include "client_utils.h"

#define REQUEST_POOL_INITIAL_SIZE 4096

ClientMode client_mode = INTERACTIVE;
ClientCommand current_command;
char *request_pool_path = "/simple_hashtable.request_pool";
int request_pool_fd = -1;
RequestPool *request_pool = NULL;

size_t request_buffer_size;
int request_buffer_shm_id;
int request_buffer_fd;
RequestHeader *request_buffer;

void clean_request_buffer() {
    close_shm_id(request_buffer_shm_id);
}

void signal_handler() {
    exit(0);
}

void expand_request_buffer(size_t target_size) {
    ftruncate(request_buffer_fd, target_size);
    request_buffer_size = target_size;
    sem_init(&request_buffer->server_post_sem, 1, 0);
}

void execute_current() {
    // build shared request body
    size_t request_body_size = sizeof(RequestHeader) + current_command.key_len + current_command.value_len;
    if (request_body_size > request_buffer_size) {
        expand_request_buffer(request_body_size);
    }

    request_buffer->request_type = current_command.type;

    request_buffer->key_len = current_command.key_len;
    char *key_buffer = request_buffer->buffer;
    memcpy(key_buffer, current_command.key, request_buffer->key_len);

    request_buffer->value_len = current_command.value_len;
    char *value_buffer = request_buffer->buffer + request_buffer->key_len;
    memcpy(value_buffer, current_command.value, request_buffer->value_len);

    // wait for an empty slot
    sem_wait(&request_pool->available_slot_sem);
    int worker_index = -1;
    bool success = false;
    while (!success) {
        for (int i = 0; i < request_pool->slot_num; i++) {
            WorkerSlot *slot = &request_pool->slots[i];
            if (!slot->available) { continue; }
            if (!atomic_fetch_and(&slot->available, false)) { continue; }
            // successfully get a slot
            worker_index = i;
            success = true;
            break;
        }
    }
    WorkerSlot *slot = &request_pool->slots[worker_index];
    slot->request_shm_id = request_buffer_shm_id;
    slot->request_size = request_body_size;
    // wake worker;
    sem_post(&slot->semaphore);

    // wait for request to be complete
    while (sem_wait(&request_buffer->server_post_sem) < 0) {
        if (errno != EINTR) {
            client_error("Error waiting for response: %s", strerror(errno));
        }
    }

    // read response
    int response_shm_id = request_buffer->response_shm_id;
    // client_log("response shm id %d", response_shm_id);
    size_t response_size = request_buffer->key_len;
    size_t response_value_size = response_size - sizeof(ResponseHeader);
    OperationResponseType response_type = request_buffer->response_type;
    bool has_response_shm = (current_command.type != INSERT) && (response_type == SUCCESS);
    ResponseHeader *response = NULL;
    int response_fd = -1;
    switch (response_type) {
    case SUCCESS: {
        if (
            (current_command.type == INSERT) ||
            (current_command.value_len == 0)
            ) {
            client_log("success");
            break;
        }
        if (response_shm_id < 0) {
            client_log("failed, get empty");
            break;
        }
        response = map_shm_id(response_shm_id, response_size, &response_fd);
        if (response == NULL) {
            client_error("Cannot map response: %s", strerror(errno));
        }
        if (
            (response_value_size == current_command.value_len) &&
            (memcmp(response->buffer, current_command.value, response_value_size) == 0)
            ) {
            client_log("success");
        } else {
            client_log("failed, expect %s get %s", current_command.value, response->buffer);
            // client_log("response value size %ld", response_value_size);
        }
    } break;
    case NO_ELEMEMT: {
        client_log("no_element");
    } break;
    default: {
        client_log("internal_error");
    } break;
    }
    if (has_response_shm) {
        sem_post(&response->client_post_sem);
        munmap(response, page_align(response_size));
        close(response_fd);
        // client_log("unmapped %p, %ld", response, page_align(response_size));
    }
}

void interactive_loop() {
    while (true) {
        char *buffer = NULL;
        scanf("%m[^\n]", &buffer);
        getc(stdin);
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
        current_command = parse_command(buffer);
        execute_current();
        free(current_command.key);
        free(current_command.value);
        free(buffer);
    }
}

int main(int argc, char **argv) {
    // initiate request buffer
    request_buffer_size = 4096;
    request_buffer = create_and_map_shm_id(request_buffer_size, &request_buffer_shm_id, &request_buffer_fd);
    atexit(clean_request_buffer);
    signal(SIGINT, signal_handler);
    sem_init(&request_buffer->server_post_sem, 1, 0);
    parse_args(argc, argv);
    // connect to request pool
    client_log("Connecting to server request pool");
    request_pool_fd = create_named_shared_fd(request_pool_path, 0);
    request_pool = mmap(NULL, REQUEST_POOL_INITIAL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, request_pool_fd, 0);
    size_t request_pool_size = sizeof(RequestPool) + sizeof(WorkerSlot) * request_pool->slot_num;
    if (request_pool_size > REQUEST_POOL_INITIAL_SIZE) {
        munmap(request_pool, REQUEST_POOL_INITIAL_SIZE);
        request_pool = mmap(NULL, request_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, request_pool_fd, 0);
    }

    switch (client_mode) {
    case INTERACTIVE: {
        client_log("Entering interactive mode");
        interactive_loop();
    } break;
    case COMMAND: {
        execute_current();
    } break;
    default: {
        client_error("Unknown client mode");
    } break;
    }
}