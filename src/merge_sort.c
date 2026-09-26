#define _POSIX_C_SOURCE 200809L
#include <string.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "timing.h"

//We set up the two typedefs for input kinds and sorting methods
typedef enum {
    INPUT_RANDOM,
    INPUT_SORTED,
    INPUT_REVERSE,
    INPUT_EQUAL
} input_kind;

typedef enum {
    METHOD_PER_CALL,
    METHOD_REUSE,
    METHOD_QSORT
} sort_method;

//This function merges two sorted subarrays. It allocates temporary space
static void merge_with_allocation(int *values, size_t left, size_t middle, size_t right) {
    size_t length = right - left;
    int *temporary = malloc(length * sizeof(*temporary));
    if (temporary == NULL) {
        fprintf(stderr, "temporary allocation failed\n");
        exit(EXIT_FAILURE);
    }

    size_t first = left;
    size_t second = middle;
    for (size_t index = 0; index < length; ++index) {
        if (first < middle && (second >= right || values[first] <= values[second])) {
            temporary[index] = values[first++];
        } else {
            temporary[index] = values[second++];
        }
    }
    memcpy(values + left, temporary, length * sizeof(*temporary));
    free(temporary);
}

//This function implements merge sort with per-call allocation
static void merge_sort_per_call_recursive(int *values, size_t left, size_t right) {
    if (right - left < 2) {
        return;
    }
    size_t middle = left + (right - left) / 2;
    merge_sort_per_call_recursive(values, left, middle);
    merge_sort_per_call_recursive(values, middle, right);
    merge_with_allocation(values, left, middle, right);
}

//This function implements merge sort with per-call allocation
static void merge_sort_per_call(int *values, size_t length) {
    merge_sort_per_call_recursive(values, 0, length);
}

//This function merges two sorted subarrays 
//It also reuses temporary space
static void merge_with_reused_array(int *values, int *temporary,
                                    size_t left, size_t middle, size_t right) {
    size_t first = left;
    size_t second = middle;
    for (size_t index = left; index < right; ++index) {
        if (first < middle && (second >= right || values[first] <= values[second])) {
            temporary[index] = values[first++];
        } else {
            temporary[index] = values[second++];
        }
    }
    memcpy(values + left, temporary + left, (right - left) * sizeof(*values));
}

//This function implements merge sort
//It also uses reused temporary space
static void merge_sort_reuse_recursive(int *values, int *temporary,
                                       size_t left, size_t right) {
    if (right - left < 2) {
        return;
    }
    size_t middle = left + (right - left) / 2;
    merge_sort_reuse_recursive(values, temporary, left, middle);
    merge_sort_reuse_recursive(values, temporary, middle, right);
    merge_with_reused_array(values, temporary, left, middle, right);
}

//This function implements merge sort
//It also uses reused temporary space
static void merge_sort_reuse(int *values, size_t length) {
    int *temporary = malloc(length * sizeof(*temporary));
    if (temporary == NULL) {
        fprintf(stderr, "temporary allocation failed\n");
        exit(EXIT_FAILURE);
    }
    merge_sort_reuse_recursive(values, temporary, 0, length);
    free(temporary);
}

//This function compares two integers for sorting
static int compare_ints(const void *left, const void *right) {
    int first = *(const int *)left;
    int second = *(const int *)right;
    return (first > second) - (first < second);
}

//This function fills the input array with values depending on the specified kind
static void fill_input(int *values, size_t length, input_kind kind) {
    uint32_t state = 123456789U;
    for (size_t index = 0; index < length; ++index) {
        switch (kind) {
            case INPUT_RANDOM:
                state = state * 1664525U + 1013904223U;
                values[index] = (int)(state & 0x7fffffffU);
                break;

            case INPUT_SORTED:
                values[index] = (int)index;
                break;

            case INPUT_REVERSE:
                values[index] = (int)(length - index);
                break;

            case INPUT_EQUAL:
                values[index] = 7;
                break;
        }
    }
}

//This will validate the output array
static int valid_output(const int *values, const int *expected, size_t length) {
    for (size_t index = 1; index < length; ++index) {
        if (values[index - 1] > values[index]) {
            return 0;
        }
    }
    return memcmp(values, expected, length * sizeof(*values)) == 0;
}

//This sorts the input array using the specified method
static void sort_values(int *values, size_t length, sort_method method) {
    if (method == METHOD_PER_CALL) {
        merge_sort_per_call(values, length);
    } else if (method == METHOD_REUSE) {
        merge_sort_reuse(values, length);
    } else {
        qsort(values, length, sizeof(*values), compare_ints);
    }
}

//Returns the name of an input kind
static const char *input_name(input_kind kind) {
    static const char *names[] = {"random", "sorted", "reverse", "equal"};
    return names[kind];
}

//Returns the name of a sorting method
static const char *method_name(sort_method method) {
    static const char *names[] = {"merge_per_call", "merge_reuse", "qsort"};
    return names[method];
}

//This function parses a positive integer from a string
static int parse_positive(const char *text, size_t *value) {
    char *end = NULL;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (end == text || *end != '\0' || parsed == 0) {
        return 0;
    }
    *value = (size_t)parsed;
    return 1;
}

//This is the main entry point for the program
//It will also print the results
int main(int argc, char **argv) {
    size_t length = 1000000;
    size_t repetitions = 1;
    if (argc == 2 || argc == 3) {
        if (!parse_positive(argv[1], &length) ||
            (argc == 3 && !parse_positive(argv[2], &repetitions))) {
            fprintf(stderr, "Usage: %s [n [repetitions]]\n", argv[0]);
            return EXIT_FAILURE;
        }
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [n [repetitions]]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int *input = malloc(length * sizeof(*input));
    int *working = malloc(length * sizeof(*working));
    int *expected = malloc(length * sizeof(*expected));
    if (input == NULL || working == NULL || expected == NULL) {
        fprintf(stderr, "array allocation failed\n");
        free(input);
        free(working);
        free(expected);
        return EXIT_FAILURE;
    }

    printf("n, input, method, repetitions, avg_us, rate_mitems_per_s, sorted_and_preserved\n");
    for (input_kind kind = INPUT_RANDOM; kind <= INPUT_EQUAL; ++kind) {
        fill_input(input, length, kind);
        memcpy(expected, input, length * sizeof(*expected));
        qsort(expected, length, sizeof(*expected), compare_ints);

        for (sort_method method = METHOD_PER_CALL; method <= METHOD_QSORT; ++method) {
            uint64_t total_ns = 0;
            int valid = 1;
            for (size_t repetition = 0; repetition < repetitions; ++repetition) {
                memcpy(working, input, length * sizeof(*working));
                uint64_t start = monotonic_ns();
                sort_values(working, length, method);
                total_ns += monotonic_ns() - start;
                valid = valid && valid_output(working, expected, length);
            }
            double average_seconds = (double)total_ns / (double)repetitions / 1e9;
            double rate = (double)length / average_seconds / 1e6;
            printf("%zu, %s, %s, %zu, %.3f, %.3f, %s\n", length, input_name(kind),
                   method_name(method), repetitions, average_seconds * 1e6,
                   rate, valid ? "yes" : "no");
            if (!valid) {
                fprintf(stderr, "%s failed validation for %s input\n",
                        method_name(method), input_name(kind));
                free(input);
                free(working);
                free(expected);
                return EXIT_FAILURE;
            }
        }
    }

    free(input);
    free(working);
    free(expected);
    return EXIT_SUCCESS;
}