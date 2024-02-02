#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "server.h"
#include "server_utils.h"
#include "hashtable.h"
#include "worker.h"
#include "shared_mem.h"

int hashtable_size = DEFAULT_HASHTABLE_SIZE;
int verbose_flag = 0;
Hashtable *server_table = NULL;
int worker_num = DEFAULT_WORKER_NUM;
RequestPool *request_pool = NULL;
WorkerNode *worker_pool = NULL;
char *request_pool_path = "/simple_hashtable.request_pool";

int main(int argc, char **argv) {
    parse_args(argc, argv);
    server_log("Setting up hashtable, size %d", hashtable_size);
    Hashtable new_table = init_hashtable(hashtable_size);
    server_log("Hashtable setup complete");
    server_table = &new_table;

    // setup worker pool
    worker_pool = malloc(sizeof(WorkerNode) * worker_num);

    // setup request pool
    size_t request_pool_mem_size = sizeof(RequestPool) + worker_num * sizeof(WorkerSlot);
    int request_pool_fd = create_named_shared_fd(request_pool_path, request_pool_mem_size);
    if (request_pool_fd < 0) {
        server_error("Request pool %s creation failed, error: %s", request_pool_path, strerror(errno));
    }
    server_log("Request pool created, fd: %d path: %s", request_pool_fd, request_pool_path);
    request_pool = map_shared_fd(request_pool_fd, request_pool_mem_size);
    server_log("Request pool mapped, address %p", request_pool);
    request_pool->slot_num = worker_num;
    if (sem_init(&request_pool->available_slot_sem, PTHREAD_PROCESS_SHARED, 0) != 0) {
        server_error("failed to initiate request pool semaphore");
    }
    server_log("Request pool semaphore initiated");

    for (int i = 0; i < worker_num; i++) {
        WorkerSlot *slot = &(request_pool->slots[i]);
        slot->available = false;
        sem_init(&slot->semaphore, PTHREAD_PROCESS_SHARED, 0);
    }
    server_log("Request pool slots initiated");

    // create workers
    for (int i = 0; i < worker_num; i++) {
        launch_worker(i);
    }
    server_log("workers launched");

    // TODO: manage worker pool according to the workload
    while (true) {
        sleep(1);
    }
}