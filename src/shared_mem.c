#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#include "shared_mem.h"

int create_shared_fd(char *path, size_t size) {
    int fd = create_named_shared_fd(path, size);
    if (fd < 0) {
        return fd;
    }

    shm_unlink(path);

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
    if (ftruncate(fd, size) < 0) {
        return -1;
    }
    return fd;
}