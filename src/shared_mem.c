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

int create_shared_fd(size_t size) {
    int fd = -1;
    char *path = malloc(0);
    while (true) {
        free(path);
        int rand_num = rand() % (1 << 20);
        asprintf(&path, "/simple_hashtable.%d", rand_num);
        fd = shm_open(path, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR | S_IROTH);
        if (fd >= 0) { break; }
        if (errno != EEXIST) { return fd; }
    }
    shm_unlink(path);
    free(path);

    if (ftruncate(fd, size) < 0) {
        return -1;
    }

    return fd;
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