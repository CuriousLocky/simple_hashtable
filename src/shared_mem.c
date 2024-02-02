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

#define PAGE_SIZE 4096
size_t page_align(size_t size) {
    return (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

void *create_and_map_shm_id(size_t size, int *shm_id_p, int *fd_p) {
    int fd = -1;
    char *path = malloc(0);
    size = page_align(size);
    while (true) {
        free(path);
        int rand_num = rand() % (1 << 25);
        asprintf(&path, "%s.%d", SHM_ID_PREFIX, rand_num);
        fd = shm_open(path, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR | S_IROTH);
        *shm_id_p = rand_num;
        if (fd_p != NULL) {
            *fd_p = fd;
        }
        if (fd >= 0) { break; }
        if (errno != EEXIST) { return NULL; }
    }
    if (ftruncate(fd, size) < 0) {
        return NULL;
    }
    char *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    // shm_unlink(path);
    free(path);
    printf("id %d created size %ld\n", *shm_id_p, size);
    return addr;
}

void *map_shm_id(int id, size_t size, int *fd_p) {
    if (id < 0) {
        return NULL;
    }
    size = page_align(size);
    char *path;
    asprintf(&path, "%s.%d", SHM_ID_PREFIX, id);
    int fd = shm_open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);
    *fd_p = fd;
    if (fd < 0) {
        free(path);
        return NULL;
    }
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    free(path);
    printf("id %d mapped size %ld\n", id, size);
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
    size = page_align(size);
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