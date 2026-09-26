#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timing.h"

#define DEFAULT_REPS 5

//This function computes the exclusive prefix sum of an array of integers
static void exclusive_prefix_sum_int(int *a, size_t n) {
    int running = 0;
    for (size_t i = 0; i < n; ++i) {
        int value = a[i];
        a[i] = running;
        running += value;
    }
}

//This function computes the exclusive prefix sum of an array of doubles
static void exclusive_prefix_sum_double(double *a, size_t n) {
    double running = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double value = a[i];
        a[i] = running;
        running += value;
    }
}

//This function fills an integer array with values
static void fill_int(int *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = (int)(i & 1U);
    }
}

//This function fills a double array with values
static void fill_double(double *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = (double)(i & 1U);
    }
}

//This prints the usage message
static void usage(const char *program) {
    fprintf(stderr, "Usage: %s [--sizes N[,N...]] [--reps N]\n", program);
}

//This function parses a list of sizes from a string
static int parse_sizes(const char *text, size_t **sizes_out, size_t *count_out) {
    char *copy = strdup(text);
    if (copy == NULL) {
        return -1;
    }

    size_t count = 1;
    for (char *cursor = copy; *cursor != '\0'; ++cursor) {
        if (*cursor == ',') {
            ++count;
        }
    }

    size_t *sizes = malloc(count * sizeof(*sizes));
    if (sizes == NULL) {
        free(copy);
        return -1;
    }

    size_t index = 0;
    char *token = strtok(copy, ",");
    while (token != NULL) {
        char *end = NULL;
        unsigned long long value = strtoull(token, &end, 10);
        if (end == token || *end != '\0' || value == 0) {
            free(sizes);
            free(copy);
            return -1;
        }
        sizes[index++] = (size_t)value;
        token = strtok(NULL, ",");
    }

    free(copy);
    *sizes_out = sizes;
    *count_out = index;
    return 0;
}

//This function benchmarks the exclusive prefix sum of an integer array
static uint64_t benchmark_int(size_t n, int reps, uint64_t *checksum) {
    int *a = malloc(n * sizeof(*a));
    if (a == NULL) {
        fprintf(stderr, "malloc failed: int array of size %zu\n", n);
        return 0;
    }

    uint64_t total_ns = 0;
    for (int repetition = 0; repetition < reps; ++repetition) {
        fill_int(a, n);
        uint64_t start = monotonic_ns();
        exclusive_prefix_sum_int(a, n);
        total_ns += monotonic_ns() - start;
    }
    *checksum = (uint64_t)(unsigned int)a[n - 1];
    free(a);
    return total_ns / (uint64_t)reps;
}

//This function benchmarks the exclusive prefix sum of a double array
static uint64_t benchmark_double(size_t n, int reps, double *checksum) {
    double *a = malloc(n * sizeof(*a));
    if (a == NULL) {
        fprintf(stderr, "malloc failed: double array of size %zu\n", n);
        return 0;
    }

    uint64_t total_ns = 0;
    for (int repetition = 0; repetition < reps; ++repetition) {
        fill_double(a, n);
        uint64_t start = monotonic_ns();
        exclusive_prefix_sum_double(a, n);
        total_ns += monotonic_ns() - start;
    }
    *checksum = a[n - 1];
    free(a);
    return total_ns / (uint64_t)reps;
}

//This is the main entry point for the program
//It will print the results
int main(int argc, char **argv) {
    static const size_t default_sizes[] = {
        1000000ULL, 10000000ULL, 100000000ULL
    };
    size_t *sizes = NULL;
    size_t size_count = sizeof(default_sizes) / sizeof(default_sizes[0]);
    int reps = DEFAULT_REPS;

    for (int argument = 1; argument < argc; ++argument) {
        if (strcmp(argv[argument], "--sizes") == 0 && argument + 1 < argc) {
            free(sizes);
            if (parse_sizes(argv[++argument], &sizes, &size_count) != 0) {
                fprintf(stderr, "invalid size list\n");
                return 1;
            }
        } else if (strcmp(argv[argument], "--reps") == 0 && argument + 1 < argc) {
            reps = atoi(argv[++argument]);
            if (reps <= 0) {
                fprintf(stderr, "needs to be positive\n");
                free(sizes);
                return 1;
            }
        } else if (strcmp(argv[argument], "--help") == 0) {
            usage(argv[0]);
            free(sizes);
            return 0;
        } else {
            usage(argv[0]);
            free(sizes);
            return 1;
        }
    }

    printf("type, n, reps, avg_us, ns_per_element, checksum\n");
    for (size_t index = 0; index < size_count; ++index) {
        size_t n = sizes == NULL ? default_sizes[index] : sizes[index];
        uint64_t int_checksum = 0;
        double double_checksum = 0.0;
        uint64_t int_ns = benchmark_int(n, reps, &int_checksum);
        uint64_t double_ns = benchmark_double(n, reps, &double_checksum);

        printf("int, %zu, %d, %.3f, %.6f, %llu\n",
               n, reps, (double)int_ns / 1000.0,
               (double)int_ns / (double)n,
               (unsigned long long)int_checksum);
        printf("double, %zu, %d, %.3f, %.6f, %.1f\n",
               n, reps, (double)double_ns / 1000.0,
               (double)double_ns / (double)n, double_checksum);
    }

    free(sizes);
    return 0;
}