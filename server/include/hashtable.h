#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <pthread.h>

typedef unsigned char byte;

typedef struct MemChunk {
    size_t len;
    byte *content;
} MemChunk;

// single linked list element for a key-value pair
typedef struct HashtableBlock_ {
    unsigned long key_hash;
    MemChunk key;
    MemChunk value;
    struct HashtableBlock_ *next;
    pthread_rwlock_t next_lock;
} HashtableBlock;

typedef struct Hashtable {
    int size;
    HashtableBlock *lists;
} Hashtable;

// init a new hashtable with given size
Hashtable init_hashtable(int size);

// insert a key-value pair into the hashtable
// if the key is already in the table, the key will be overwritten
int hashtable_insert(Hashtable *table, MemChunk key, MemChunk value);

// search the value to of a key
// if not found, return chunk with NULL content
MemChunk hashtable_search(Hashtable *table, MemChunk key);

// delete the value of a key
// if was not found, return chunk with NULL content
MemChunk hashtable_delete(Hashtable *table, MemChunk key);

#endif