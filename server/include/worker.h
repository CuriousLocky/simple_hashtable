#ifndef WORKER_H
#define WORKER_H

#include <semaphore.h>
#include <stdbool.h>

#include "hashtable.h"
#include "protocol.h"

typedef struct {
    pthread_t thread_id;
} WorkerNode;

void launch_worker(int id);

#endif