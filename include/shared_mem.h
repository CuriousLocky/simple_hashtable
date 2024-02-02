#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef struct {
    int fd;
    char *addr;
} SharedMem;

size_t page_align(size_t size);
void *create_and_map_shm_id(size_t size, int *shm_id_p, int *fd_p);
void *map_shm_id(int id, size_t size, int *fd_p);
int close_shm_id(int id);
int create_named_shared_fd(char *path, size_t size);

#endif
