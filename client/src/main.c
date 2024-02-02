#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <errno.h>

#include "shared_mem.h"
#include "client.h"
#include "client_utils.h"

#define REQUEST_POOL_INITIAL_SIZE 4096

ClientMode client_mode = INTERACTIVE;
ClientCommand current_command;
char *request_pool_path = "/simple_hashtable.request_pool";
int request_pool_fd = -1;
RequestPool *request_pool = NULL;

void execute_current() {
    // build shared request body
    size_t request_body_size = sizeof(RequestHeader) + current_command.key_len + current_command.value_len;
    int request_shm_id = -1;
    RequestHeader *request_body = create_and_map_shm_id(request_body_size, &request_shm_id);

    request_body->request_type = current_command.type;
    sem_init(&request_body->semaphore, 1, 0);

    request_body->key_len = current_command.key_len;
    char *key_buffer = request_body->request_buffer;
    memcpy(key_buffer, current_command.key, request_body->key_len);

    request_body->value_len = current_command.value_len;
    char *value_buffer = request_body->request_buffer + request_body->key_len;
    memcpy(value_buffer, current_command.value, request_body->value_len);

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
    slot->request_shm_id = request_shm_id;
    slot->request_size = request_body_size;
    // wake worker;
    sem_post(&slot->semaphore);

    // wait for request to be complete
    while (sem_wait(&request_body->semaphore) < 0) {
        if (errno != EINTR) {
            client_error("Error waiting for response: %s", strerror(errno));
        }
    }

    // read response
    int response_shm_id = request_body->response_shm_id;
    size_t response_size = request_body->key_len;
    OperationResponseType response_type = request_body->response_type;
    bool has_response_shm = (current_command.type != INSERT) && (response_type == SUCCESS);
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
        char *response_body = map_shm_id(response_shm_id, response_size);
        if (response_body == NULL) {
            client_error("Cannot map response: %s", strerror(errno));
        }
        if (
            (response_size == current_command.value_len) &&
            (memcmp(response_body, current_command.value, response_size) == 0)
            ) {
            client_log("success");
        } else {
            client_log("failed, value different");
        }
        munmap(response_body, response_size);
    } break;
    case NO_ELEMEMT: {
        client_log("no_element");
    } break;
    default: {
        client_log("internal_error");
    } break;
    }
    // print_current();
    if (has_response_shm) {
        close_shm_id(response_shm_id);
    }
    munmap(request_body, request_body_size);
    close_shm_id(request_shm_id);
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