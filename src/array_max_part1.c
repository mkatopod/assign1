#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timing.h"

#define DEFAULT_REPS 12
#define SWEEP_MIN_BYTES 2048ULL
#define SWEEP_MAX_BYTES (256ULL * 1024ULL * 1024ULL)

typedef enum {
    ORDER_SORTED,
    ORDER_REVERSE,
    ORDER_RANDOM
} input_order_t;

typedef struct {
    size_t n;
    input_order_t order;
    int type; 
    int variant; 
    uint64_t avg_ns;
    uint64_t median_ns;
    uint64_t min_ns;
    uint64_t max_ns;
    double avg_bytes_per_s;
    double avg_ns_per_elem;
} bench_result_t;

//This displays usage info for the program
static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s [--reps N] [--sizes 1000000,10000000,100000000] [--orders sorted,reverse,random]\n"
            "       %s --sweep [--reps N]\n",
            prog, prog);
}

//This fills an array with sorted integers
static void fill_sorted_int(int *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = (int)i % 131071;
    }
}

//This fills an array with reverse-sorted integers
static void fill_reverse_int(int *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = (int)(n - i) % 131071;
    }
}

//This fills an array with random integers
static void fill_random_int(int *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = (int)((i * 1103515245u + 12345u) & 0x7fffffff);
    }
}

//This fills an array with sorted doubles
static void fill_sorted_double(double *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = (double)((i % 131071) + 0.25);
    }
}

//This fills an array with reverse-sorted doubles
static void fill_reverse_double(double *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = (double)((n - i) % 131071 + 0.25);
    }
}

//This fills an array with random doubles
static void fill_random_double(double *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = (double)(((i * 1103515245u + 12345u) & 0x7fffffff) % 131071) + 0.25;
    }
}

//This calculates the median of an array of uint64_t values
static uint64_t median_u64(uint64_t *values, size_t count) {
    if (count == 0) return 0;
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            if (values[j] < values[i]) {
                uint64_t tmp = values[i];
                values[i] = values[j];
                values[j] = tmp;
            }
        }
    }
    return values[count / 2];
}

//This finds the maximum value in an array of integers for a
static int max_int_a(const int *a, size_t n) {
    int m = a[0];
    for (size_t i = 1; i < n; ++i) {
        if (a[i] > m) {
            m = a[i];
        }
    }
    return m;
}

//This finds the maximum value in an array of integers for b
static int max_int_b(const int *a, size_t n) {
    int m = a[0];
    for (size_t i = 1; i < n; ++i) {
        m = (a[i] > m) ? a[i] : m;
    }
    return m;
}

//This finds the maximum value in an array of doubles for a
static double max_double_a(const double *a, size_t n) {
    double m = a[0];
    for (size_t i = 1; i < n; ++i) {
        if (a[i] > m) {
            m = a[i];
        }
    }
    return m;
}

//This finds the maximum value in an array of doubles for b
static double max_double_b(const double *a, size_t n) {
    double m = a[0];
    for (size_t i = 1; i < n; ++i) {
        m = (a[i] > m) ? a[i] : m;
    }
    return m;
}

//In the static, this parses size specifications from a comma-separated string
static int parse_sizes(const char *text, size_t **sizes_out, size_t *count_out) {
    if (text == NULL || *text == '\0') {
        return 0;
    }

    char *copy = strdup(text);
    if (copy == NULL) {
        return -1;
    }

    size_t cap = 8;
    size_t count = 0;
    size_t *sizes = malloc(cap * sizeof(*sizes));
    if (sizes == NULL) {
        free(copy);
        return -1;
    }

    char *tok = strtok(copy, ",");
    while (tok != NULL) {
        if (count == cap) {
            cap *= 2;
            size_t *new_sizes = realloc(sizes, cap * sizeof(*sizes));
            if (new_sizes == NULL) {
                free(sizes);
                free(copy);
                return -1;
            }
            sizes = new_sizes;
        }

        char *end = NULL;
        unsigned long long value = strtoull(tok, &end, 10);
        if (end == tok || *end != '\0') {
            free(sizes);
            free(copy);
            return -1;
        }

        sizes[count++] = (size_t)value;
        tok = strtok(NULL, ",");
    }

    free(copy);
    *sizes_out = sizes;
    *count_out = count;
    return 0;
}

//In this static, it parses input order specifications from a comma-separated string
static int parse_orders(const char *text, input_order_t **orders_out, size_t *count_out) {
    if (text == NULL || *text == '\0') {
        return 0;
    }

    char *copy = strdup(text);
    if (copy == NULL) {
        return -1;
    }

    size_t cap = 4;
    size_t count = 0;
    input_order_t *orders = malloc(cap * sizeof(*orders));
    if (orders == NULL) {
        free(copy);
        return -1;
    }

    char *tok = strtok(copy, ",");
    while (tok != NULL) {
        if (count == cap) {
            cap *= 2;
            input_order_t *new_orders = realloc(orders, cap * sizeof(*orders));
            if (new_orders == NULL) {
                free(orders);
                free(copy);
                return -1;
            }
            orders = new_orders;
        }

        if (strcmp(tok, "sorted") == 0) {
            orders[count++] = ORDER_SORTED;
        } else if (strcmp(tok, "reverse") == 0) {
            orders[count++] = ORDER_REVERSE;
        } else if (strcmp(tok, "random") == 0) {
            orders[count++] = ORDER_RANDOM;
        } else {
            free(orders);
            free(copy);
            return -1;
        }

        tok = strtok(NULL, ",");
    }

    free(copy);
    *orders_out = orders;
    *count_out = count;
    return 0;
}

//Returns the name of an input order
static const char *order_name(input_order_t order) {
    switch (order) {
        case ORDER_SORTED: return "sorted";
        case ORDER_REVERSE: return "reverse";
        case ORDER_RANDOM: return "random";
        default: return "unknown";
    }
}

//Returns the name of a data type
static const char *type_name(int type) {
    return type == 0 ? "int" : "double";
}

//Returns the name of a variant
static const char *variant_name(int variant) {
    return variant == 0 ? "A" : "B";
}

//This benchmarks the integer array maximum functions
//It will also print the benchmark results
static void benchmark_int(size_t n, input_order_t order, int variant, int reps, bench_result_t *result) {
    int *a = malloc(n * sizeof(*a));
    if (a == NULL) {
        fprintf(stderr, "malloc failed for int array of size %zu\n", n);
        exit(1);
    }

    switch (order) {
        case ORDER_SORTED: fill_sorted_int(a, n); break;
        case ORDER_REVERSE: fill_reverse_int(a, n); break;
        case ORDER_RANDOM: fill_random_int(a, n); break;
        default: break;
    }

    uint64_t times[DEFAULT_REPS] = {0};
    int warmup_result = 0;
    int result_value = 0;

    if (variant == 0) {
        warmup_result = max_int_a(a, n);
    } else {
        warmup_result = max_int_b(a, n);
    }

    result_value = warmup_result;
    for (int r = 0; r < reps; ++r) {
        uint64_t start = monotonic_ns();
        if (variant == 0) {
            result_value = max_int_a(a, n);
        } else {
            result_value = max_int_b(a, n);
        }
        uint64_t end = monotonic_ns();
        times[r] = end - start;
    }

    uint64_t sum = 0;
    uint64_t min = times[0];
    uint64_t max = times[0];
    for (int r = 0; r < reps; ++r) {
        sum += times[r];
        if (times[r] < min) min = times[r];
        if (times[r] > max) max = times[r];
    }

    result->n = n;
    result->type = 0;
    result->variant = variant;
    result->avg_ns = sum / (uint64_t)reps;
    result->median_ns = median_u64(times, (size_t)reps);
    result->min_ns = min;
    result->max_ns = max;
    result->avg_ns_per_elem = (double)result->avg_ns / (double)n;
    result->avg_bytes_per_s = ((double)n * sizeof(int)) / ((double)result->avg_ns / 1e9);

    printf("%s %s %s n=%zu reps=%d avg=%.3f us median=%.3f us min=%.3f us max=%.3f us rate=%.3f MB/s ns/elem=%.3f result=%d\n",
           type_name(result->type),
           variant_name(result->variant),
           order_name(order),
           n,
           reps,
           (double)result->avg_ns / 1000.0,
           (double)result->median_ns / 1000.0,
           (double)result->min_ns / 1000.0,
           (double)result->max_ns / 1000.0,
           result->avg_bytes_per_s / (1024.0 * 1024.0),
           result->avg_ns_per_elem,
           result_value);

    free(a);
}

//This benchmarks the double array maximum functions
//It also prints the benchmark results
static void benchmark_double(size_t n, input_order_t order, int variant, int reps,
                             int csv, bench_result_t *result) {
    double *a = malloc(n * sizeof(*a));
    if (a == NULL) {
        fprintf(stderr, "malloc failed for double array of size %zu\n", n);
        exit(1);
    }

    switch (order) {
        case ORDER_SORTED: fill_sorted_double(a, n); break;
        case ORDER_REVERSE: fill_reverse_double(a, n); break;
        case ORDER_RANDOM: fill_random_double(a, n); break;
        default: break;
    }

    uint64_t *times = calloc((size_t)reps, sizeof(*times));
    if (times == NULL) {
        fprintf(stderr, "malloc failed for timing samples\n");
        free(a);
        exit(1);
    }
    double warmup_result = 0.0;
    double result_value = 0.0;

    if (variant == 0) {
        warmup_result = max_double_a(a, n);
    } else {
        warmup_result = max_double_b(a, n);
    }

    result_value = warmup_result;
    for (int r = 0; r < reps; ++r) {
        uint64_t start = monotonic_ns();
        if (variant == 0) {
            result_value = max_double_a(a, n);
        } else {
            result_value = max_double_b(a, n);
        }
        uint64_t end = monotonic_ns();
        times[r] = end - start;
    }

    uint64_t sum = 0;
    uint64_t min = times[0];
    uint64_t max = times[0];
    for (int r = 0; r < reps; ++r) {
        sum += times[r];
        if (times[r] < min) min = times[r];
        if (times[r] > max) max = times[r];
    }

    result->n = n;
    result->type = 1;
    result->variant = variant;
    result->avg_ns = sum / (uint64_t)reps;
    result->median_ns = median_u64(times, (size_t)reps);
    result->min_ns = min;
    result->max_ns = max;
    result->avg_ns_per_elem = (double)result->avg_ns / (double)n;
    result->avg_bytes_per_s = ((double)n * sizeof(double)) / ((double)result->avg_ns / 1e9);

    if (csv) {
        printf("%zu,%.3f,%.3f,%.3f,%.6f,%.6f\n",
               n * sizeof(double),
               result->avg_bytes_per_s,
               (double)result->avg_ns / 1000.0,
               (double)result->median_ns / 1000.0,
               result->avg_ns_per_elem,
               result_value);
    } else {
        printf("%s %s %s n=%zu reps=%d avg=%.3f us median=%.3f us min=%.3f us max=%.3f us rate=%.3f MB/s ns/elem=%.3f result=%.6f\n",
               type_name(result->type),
               variant_name(result->variant),
               order_name(order),
               n,
               reps,
               (double)result->avg_ns / 1000.0,
               (double)result->median_ns / 1000.0,
               (double)result->min_ns / 1000.0,
               (double)result->max_ns / 1000.0,
               result->avg_bytes_per_s / (1024.0 * 1024.0),
               result->avg_ns_per_elem,
               result_value);
    }

    free(times);
    free(a);
}

int main(int argc, char **argv) {
    size_t *sizes = NULL;
    size_t size_count = 0;
    int owns_sizes = 0;
    input_order_t *orders = NULL;
    size_t order_count = 0;
    int owns_orders = 0;
    int reps = DEFAULT_REPS;
    int sweep = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--reps") == 0 && i + 1 < argc) {
            reps = atoi(argv[++i]);
            if (reps <= 0) {
                fprintf(stderr, "reps must be positive\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--sizes") == 0 && i + 1 < argc) {
            if (parse_sizes(argv[++i], &sizes, &size_count) != 0) {
                fprintf(stderr, "invalid size list\n");
                return 1;
            }
            owns_sizes = 1;
        } else if (strcmp(argv[i], "--orders") == 0 && i + 1 < argc) {
            if (parse_orders(argv[++i], &orders, &order_count) != 0) {
                fprintf(stderr, "invalid order list\n");
                return 1;
            }
            owns_orders = 1;
        } else if (strcmp(argv[i], "--sweep") == 0) {
            sweep = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (sweep) {
        printf("array_bytes,bandwidth_bytes_per_s,avg_us,median_us,ns_per_element,result\n");
        for (size_t bytes = SWEEP_MIN_BYTES; bytes <= SWEEP_MAX_BYTES; bytes *= 2) {
            bench_result_t result;
            benchmark_double(bytes / sizeof(double), ORDER_RANDOM, 1, reps, 1, &result);
        }
        return 0;
    }

    if (sizes == NULL) {
        static const size_t default_sizes[] = {1000000ULL, 10000000ULL, 100000000ULL};
        sizes = (size_t *)default_sizes;
        size_count = sizeof(default_sizes) / sizeof(default_sizes[0]);
    }

    if (orders == NULL) {
        static const input_order_t default_orders[] = {ORDER_SORTED, ORDER_REVERSE, ORDER_RANDOM};
        orders = (input_order_t *)default_orders;
        order_count = sizeof(default_orders) / sizeof(default_orders[0]);
    }

    for (size_t i = 0; i < size_count; ++i) {
        for (size_t j = 0; j < order_count; ++j) {
            bench_result_t r_int_a, r_int_b, r_double_a, r_double_b;
            benchmark_int(sizes[i], orders[j], 0, reps, &r_int_a);
            benchmark_int(sizes[i], orders[j], 1, reps, &r_int_b);
            benchmark_double(sizes[i], orders[j], 0, reps, 0, &r_double_a);
            benchmark_double(sizes[i], orders[j], 1, reps, 0, &r_double_b);
            (void)r_int_a; (void)r_int_b; (void)r_double_a; (void)r_double_b;
        }
    }

    if (owns_sizes) {
        free(sizes);
    }
    if (owns_orders) {
        free(orders);
    }
    return 0;
}
