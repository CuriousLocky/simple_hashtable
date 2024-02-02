#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef struct {
    int fd;
    char *addr;
} SharedMem;

int create_shared_fd(char *path, size_t size);
void *map_shared_fd(int fd, size_t size);
int create_named_shared_fd(char *path, size_t size);

#endif
