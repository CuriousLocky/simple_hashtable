#define _GNU_SOURCE
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "shared_mem.h"

void *create_and_map_shared_fd(size_t size, int *shm_fd) {
    int fd = -1;
    char *path = malloc(0);
    while (true) {
        free(path);
        int rand_num = rand() % (1 << 20);
        asprintf(&path, "/simple_hashtable.%d", rand_num);
        fd = shm_open(path, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR | S_IROTH);
        *shm_fd = fd;
        if (fd >= 0) { break; }
        if (errno != EEXIST) { return NULL; }
    }
    if (ftruncate(fd, size) < 0) {
        return NULL;
    }
    char *addr = map_shared_fd(fd, size);
    // shm_unlink(path);
    free(path);


    return addr;
}

void *map_shared_fd(int fd, size_t size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

int create_named_shared_fd(char *path, size_t size) {
    int fd = shm_open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);
    if (fd < 0) {
        return fd;
    }
    // if size is zero, do not zero it to set size
    if (size != 0) {
        if (ftruncate(fd, size) < 0) {
            return -1;
        }
    }
    return fd;
}