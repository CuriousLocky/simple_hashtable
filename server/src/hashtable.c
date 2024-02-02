#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "hashtable.h"
#include "server_utils.h"


Hashtable init_hashtable(int size) {
    Hashtable new_table;
    new_table.size = size;
    new_table.lists = malloc(sizeof(HashtableBlock) * size);
    // setup dummy list heads
    for (int i = 0; i < size; i++) {
        new_table.lists[i].next = NULL;
        pthread_rwlock_init(&new_table.lists[i].next_lock, NULL);
    }
    return new_table;
}

// a hashing method introduced in http://www.cse.yorku.ca/~oz/hash.html
static unsigned long hash(MemChunk key) {
    size_t size = key.len;
    byte *content = key.content;
    unsigned long hash = 5381;
    for (size_t i = 0; i < size; i++) {
        hash = ((hash << 5) + hash) + content[i];
    }
    return hash;
}

// compare a new key with an entry
static int compare(unsigned long entry_hash, MemChunk entry_key, unsigned long key_hash, MemChunk key) {
    if (entry_hash != key_hash) {
        return entry_hash - key_hash;
    }
    if (entry_key.len != key.len) {
        return entry_key.len - key.len;
    }
    for (size_t i = 0; i < key.len; i++) {
        if (entry_key.content[i] != key.content[i]) {
            return entry_key.content[i] - key.content[i];
        }
    }
    return 0;
}

// return a block whose next_lock is rdlocked next is the target position
// if not found, the returned block has NULL as next
static HashtableBlock *rdlock_and_walk(Hashtable *table, MemChunk key) {
    unsigned long key_hash = hash(key);
    int slot = key_hash % (table->size);
    int lock_error = 0;

    HashtableBlock *curr = &table->lists[slot];
    HashtableBlock *next = NULL;
    lock_error = pthread_rwlock_rdlock(&curr->next_lock);
    if (lock_error != 0) {
        server_error("read lock error %d, slot %d, key length %d", lock_error, slot, key.len);
    }
    next = curr->next;

    while ((next != NULL) && (compare(next->key_hash, next->key, key_hash, key) != 0)) {
        lock_error = pthread_rwlock_rdlock(&next->next_lock);
        if (lock_error != 0) {
            server_error("read lock error %d, slot %d, key length %d", lock_error, slot, key.len);
        }
        lock_error = pthread_rwlock_unlock(&curr->next_lock);
        if (lock_error != 0) {
            server_error("unlock error %d, slot %d, key length %d", lock_error, slot, key.len);
        }
        curr = next;
        next = curr->next;
    }
    return curr;
}

// return a block whose next_lock is wrlocked next is the target position
// if not found, the returned block has NULL as next
static HashtableBlock *wrlock_and_walk(Hashtable *table, MemChunk key) {
    unsigned long key_hash = hash(key);
    int slot = key_hash % (table->size);
    int lock_error = 0;

    HashtableBlock *curr = &table->lists[slot];
    HashtableBlock *next = NULL;
    lock_error = pthread_rwlock_wrlock(&curr->next_lock);
    if (lock_error != 0) {
        server_error("write lock error %d, slot %d, key length %d", lock_error, slot, key.len);
    }
    next = curr->next;

    while ((next != NULL) && (compare(next->key_hash, next->key, key_hash, key) < 0)) {
        lock_error = pthread_rwlock_wrlock(&next->next_lock);
        if (lock_error != 0) {
            server_error("write lock error %d, slot %d, key length %d", lock_error, slot, key.len);
        }
        lock_error = pthread_rwlock_unlock(&curr->next_lock);
        if (lock_error != 0) {
            server_error("unlock error %d, slot %d, key length %d", lock_error, slot, key.len);
        }
        curr = next;
        next = curr->next;
    }
    return curr;
}

// insert a key-value pair into the hashtable
// if the key is already in the table, the key will be overwritten
int hashtable_insert(Hashtable *table, MemChunk key, MemChunk value) {
    unsigned long key_hash = hash(key);
    HashtableBlock *target_prev = wrlock_and_walk(table, key);
    HashtableBlock *target_block = target_prev->next;
    if (
        (target_block != NULL) &&
        (compare(target_block->key_hash, target_block->key, key_hash, key) == 0)
        ) {
        // key already in list, change value
        free(target_block->value.content);
        target_block->value.len = value.len;
        target_block->value.content = value.content;
        pthread_rwlock_unlock(&target_prev->next_lock);
        return 0;
    }
    // key not in list, create new block and insert
    HashtableBlock *new_block = malloc(sizeof(HashtableBlock));
    new_block->key = key;
    new_block->key_hash = key_hash;
    new_block->value = value;
    new_block->next = target_block;
    pthread_rwlock_init(&new_block->next_lock, NULL);
    target_prev->next = new_block;
    pthread_rwlock_unlock(&target_prev->next_lock);
    return 0;
}

// search the value to of a key
// if not found, return chunk with NULL content
MemChunk hashtable_search(Hashtable *table, MemChunk key) {
    MemChunk result = { 0, NULL };
    HashtableBlock *target_prev = rdlock_and_walk(table, key);
    HashtableBlock *target = target_prev->next;
    if (target == NULL) {
        pthread_rwlock_unlock(&target_prev->next_lock);
        free(key.content);
        return result;
    }
    unsigned long key_hash = hash(key);
    result.len = target->value.len;
    result.content = malloc(result.len);
    memcpy(result.content, target->value.content, result.len);
    pthread_rwlock_unlock(&target_prev->next_lock);
    free(key.content);
    return result;
}

// delete the value of a key
// if was not found, return chunk with NULL content
MemChunk hashtable_delete(Hashtable *table, MemChunk key) {
    unsigned long key_hash = hash(key);
    MemChunk result = { 0, NULL };
    HashtableBlock *target_prev = wrlock_and_walk(table, key);
    HashtableBlock *target_block = target_prev->next;
    if (
        (target_block == NULL) ||
        (compare(target_block->key_hash, target_block->key, key_hash, key) != 0)
        ) {
        free(key.content);
        pthread_rwlock_unlock(&target_prev->next_lock);
        return result;
    }
    free(key.content);
    pthread_rwlock_wrlock(&target_block->next_lock);
    target_prev->next = target_block->next;
    pthread_rwlock_unlock(&target_block->next_lock);
    pthread_rwlock_unlock(&target_prev->next_lock);
    // deleted from list
    result = target_block->value;
    pthread_rwlock_destroy(&target_block->next_lock);
    free(target_block);
    return result;
}
