#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <stdint.h>

static inline uint64_t monotonic_ns(void) {
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    return (uint64_t)timestamp.tv_sec * 1000000000ULL + (uint64_t)timestamp.tv_nsec;
}
