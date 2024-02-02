#ifndef SERVER_H
#define SERVER_H

#include "hashtable.h"
#include "protocol.h"
#include "worker.h"

#define DEFAULT_HASHTABLE_SIZE 32

extern int verbose_flag;
extern Hashtable *server_table;
extern RequestPool *request_pool;
extern WorkerNode *worker_pool;

#endif