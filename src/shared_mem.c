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
#include "protocol.h"

void *create_and_map_shm_id(size_t size, int *shm_id) {
    int fd = -1;
    char *path = malloc(0);
    while (true) {
        free(path);
        int rand_num = rand() % (1 << 25);
        asprintf(&path, "%s.%d", SHM_ID_PREFIX, rand_num);
        fd = shm_open(path, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR | S_IROTH);
        *shm_id = rand_num;
        if (fd >= 0) { break; }
        if (errno != EEXIST) { return NULL; }
    }
    if (ftruncate(fd, size) < 0) {
        return NULL;
    }
    char *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    // shm_unlink(path);
    free(path);

    return addr;
}

void *map_shm_id(int id, size_t size) {
    if (id < 0) {
        return NULL;
    }
    char *path;
    asprintf(&path, "%s.%d", SHM_ID_PREFIX, id);
    int fd = shm_open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);
    if (fd < 0) {
        free(path);
        return NULL;
    }
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    free(path);
    return addr;
}

int close_shm_id(int id) {
    char *path;
    asprintf(&path, "%s.%d", SHM_ID_PREFIX, id);
    int result = shm_unlink(path);
    free(path);
    return result;
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