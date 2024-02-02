#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef struct {
    int fd;
    char *addr;
} SharedMem;

void *create_and_map_shm_id(size_t size, int *shm_id);
void *map_shm_id(int id, size_t size);
// void *map_shared_fd(int fd, size_t size);
int create_named_shared_fd(char *path, size_t size);

#endif
