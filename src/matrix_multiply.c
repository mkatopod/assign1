#define _POSIX_C_SOURCE 200809L
#include <string.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "timing.h"

//We set typedef for the matrix multiplication functions
typedef void (*multiply_function)(const int *, const int *, int *, size_t, size_t, size_t);

//we set typedef for multiply order
typedef struct {
    const char *name;
    multiply_function function;
} multiply_order;

//This will clear a matrix
static void clear_matrix(int *matrix, size_t elements) {
    memset(matrix, 0, elements * sizeof(*matrix));
}

//This function multiplies two matrices in the order IJK
static void multiply_ijk(const int *a, const int *b, int *c, size_t m, size_t k, size_t n) {
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t inner = 0; inner < k; ++inner) {
                c[i * n + j] += a[i * k + inner] * b[inner * n + j];
            }
        }
    }
}

//This function multiplies two matrices in the order IKJ
static void multiply_ikj(const int *a, const int *b, int *c, size_t m, size_t k, size_t n) {
    for (size_t i = 0; i < m; ++i) {
        for (size_t inner = 0; inner < k; ++inner) {
            for (size_t j = 0; j < n; ++j) {
                c[i * n + j] += a[i * k + inner] * b[inner * n + j];
            }
        }
    }
}

//This function multiplies two matrices in the order JIK
static void multiply_jik(const int *a, const int *b, int *c, size_t m, size_t k, size_t n) {
    for (size_t j = 0; j < n; ++j) {
        for (size_t i = 0; i < m; ++i) {
            for (size_t inner = 0; inner < k; ++inner) {
                c[i * n + j] += a[i * k + inner] * b[inner * n + j];
            }
        }
    }
}

//This function multiplies two matrices in the order JKI
static void multiply_jki(const int *a, const int *b, int *c, size_t m, size_t k, size_t n) {
    for (size_t j = 0; j < n; ++j) {
        for (size_t inner = 0; inner < k; ++inner) {
            for (size_t i = 0; i < m; ++i) {
                c[i * n + j] += a[i * k + inner] * b[inner * n + j];
            }
        }
    }
}

//This function multiplies two matrices in the order KIJ
static void multiply_kij(const int *a, const int *b, int *c, size_t m, size_t k, size_t n) {
    for (size_t inner = 0; inner < k; ++inner) {
        for (size_t i = 0; i < m; ++i) {
            for (size_t j = 0; j < n; ++j) {
                c[i * n + j] += a[i * k + inner] * b[inner * n + j];
            }
        }
    }
}

//This function multiplies two matrices in the order KJI
static void multiply_kji(const int *a, const int *b, int *c, size_t m, size_t k, size_t n) {
    for (size_t inner = 0; inner < k; ++inner) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t i = 0; i < m; ++i) {
                c[i * n + j] += a[i * k + inner] * b[inner * n + j];
            }
        }
    }
}

//The static function compares two matrices for equality
static int matrices_equal(const int *left, const int *right, size_t elements) {
    for (size_t index = 0; index < elements; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

//This static function fills a matrix with values
static void fill_matrix(int *matrix, size_t elements, unsigned int seed) {
    for (size_t index = 0; index < elements; ++index) {
        matrix[index] = (int)((index * 17U + seed * 13U) % 9U) + 1;
    }
}

//We then do parse positive integers
static int parse_positive(const char *text, size_t *value) {
    char *end = NULL;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (end == text || *end != '\0' || parsed == 0) {
        return 0;
    }
    *value = (size_t)parsed;
    return 1;
}

//In the main, it reads matrix dimensions from the command line and perform matrix multiplication
//In the end, it prints the results
int main(int argc, char **argv) {
    size_t m = 256;
    size_t k = 256;
    size_t n = 256;
    size_t reps = 3;
    if ((argc == 4 || argc == 5) && parse_positive(argv[1], &m) &&
        parse_positive(argv[2], &k) && parse_positive(argv[3], &n)) {
        if (argc == 5 && !parse_positive(argv[4], &reps)) {
            fprintf(stderr, "dimensions/reps need to be positive integers\n");
            return 1;
        }
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [M K N [reps]]\n", argv[0]);
        return 1;
    }

    size_t a_elements = m * k;
    size_t b_elements = k * n;
    size_t c_elements = m * n;
    int *a = malloc(a_elements * sizeof(*a));
    int *b = malloc(b_elements * sizeof(*b));
    int *reference = malloc(c_elements * sizeof(*reference));
    int *result = malloc(c_elements * sizeof(*result));
    if (a == NULL || b == NULL || reference == NULL || result == NULL) {
        fprintf(stderr, "failed\n");
        free(a);
        free(b);
        free(reference);
        free(result);
        return 1;
    }

    fill_matrix(a, a_elements, 1);
    fill_matrix(b, b_elements, 2);
    clear_matrix(reference, c_elements);
    multiply_ijk(a, b, reference, m, k, n);

    const multiply_order orders[] = {
        {"ijk", multiply_ijk}, {"ikj", multiply_ikj},
        {"jik", multiply_jik}, {"jki", multiply_jki},
        {"kij", multiply_kij}, {"kji", multiply_kji}
    };
    size_t order_count = sizeof(orders) / sizeof(orders[0]);

    printf("M, K, N, order, reps, avg_us, gflops, correct\n");
    for (size_t order_index = 0; order_index < order_count; ++order_index) {
        uint64_t total_ns = 0;
        int correct = 1;
        for (size_t repetition = 0; repetition < reps; ++repetition) {
            clear_matrix(result, c_elements);
            uint64_t start = monotonic_ns();
            orders[order_index].function(a, b, result, m, k, n);
            total_ns += monotonic_ns() - start;
            if (!matrices_equal(reference, result, c_elements)) {
                correct = 0;
            }
        }
        double average_seconds = (double)total_ns / (double)reps / 1e9;
        double gflops = (2.0 * (double)m * (double)k * (double)n) /
                        average_seconds / 1e9;
                        
        printf("%zu, %zu, %zu, %s, %zu, %.3f, %.6f, %s\n",
               m, k, n, orders[order_index].name, reps,
               average_seconds * 1e6, gflops, correct ? "yes" : "no");
        if (!correct) {
             fprintf(stderr, "%s produced an incorrect result\n", orders[order_index].name);
            free(a);
            free(b);
            free(reference);
            free(result);
            return 1;
        }
    }

    free(a);
    free(b);
    free(reference);
    free(result);
    return 0;
}
