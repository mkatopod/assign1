#define _POSIX_C_SOURCE 200809L
#include <string.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "timing.h"

//This typedef defines the graph data structure
typedef struct {
    int n;
    long m;
    long *offsets;
    int *adj;
} graph_t;

//This typedef defines the result structure for the BFS computation
typedef struct {
    long inspected_edges;
    int levels;
    int reached;
    uint64_t elapsed_ns;
} bfs_result;

//This typedef defines an edge
typedef struct {
    int from;
    int to;
} edge_t;

//We start by defining a function; should handle failures
static void fail(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

//This function reads the next data line from a file, skipping comment lines
static int next_data_line(FILE *file, char *line, size_t capacity) {
    while (fgets(line, (int)capacity, file) != NULL) {
        if (line[0] != '%' && line[0] != '\n' && line[0] != '\0') {
            return 1;
        }
    }
    return 0;
}

//This function compares two edges; used for sorting
static int edge_compare(const void *left, const void *right) {
    const edge_t *first = left;
    const edge_t *second = right;
    if (first->from != second->from) {
        return first->from - second->from;
    }
    return first->to - second->to;
}

//This function reads a matrix market file
//It also constructs a graph
static graph_t read_matrix_market(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    char line[512];
    if (fgets(line, sizeof(line), file) == NULL ||
        strncmp(line, "%%MatrixMarket matrix coordinate", 32) != 0) {
        fclose(file);
        fail("input is not a coordinate file");
    }

    int n = 0;
    long declared_edges = 0;
    if (!next_data_line(file, line, sizeof(line)) ||
        sscanf(line, "%d %*d %ld", &n, &declared_edges) != 2 ||
        n <= 0 || declared_edges < 0) {
        fclose(file);
        fail("invalid dimensions");
    }

    edge_t *edges = malloc((size_t)declared_edges * 2U * sizeof(*edges));
    if (declared_edges > 0 && edges == NULL) {
        fclose(file);
        fail("edge allocation failed");
    }

    long edge_count = 0;
    for (long index = 0; index < declared_edges; ++index) {
        int from = 0;
        int to = 0;
        if (!next_data_line(file, line, sizeof(line)) ||
            sscanf(line, "%d %d", &from, &to) != 2 ||
            from < 1 || from > n || to < 1 || to > n || from == to) {
            free(edges);
            fclose(file);
            fail("invalid edge");
        }
        edges[edge_count++] = (edge_t){from - 1, to - 1};
        edges[edge_count++] = (edge_t){to - 1, from - 1};
    }
    fclose(file);

    qsort(edges, (size_t)edge_count, sizeof(*edges), edge_compare);
    edge_t *unique_edges = malloc((size_t)edge_count * sizeof(*unique_edges));
    if (edge_count > 0 && unique_edges == NULL) {
        free(edges);
        fail("unique edge allocation failed");
    }
    long unique_count = 0;
    for (long index = 0; index < edge_count; ++index) {
        if (unique_count == 0 || edge_compare(&edges[index], &unique_edges[unique_count - 1]) != 0) {
            unique_edges[unique_count++] = edges[index];
        }
    }
    free(edges);

    graph_t graph = {0};
    graph.n = n;
    graph.m = unique_count;
    graph.offsets = calloc((size_t)n + 1U, sizeof(*graph.offsets));
    graph.adj = malloc((size_t)unique_count * sizeof(*graph.adj));
    if (graph.offsets == NULL || (unique_count > 0 && graph.adj == NULL)) {
        free(unique_edges);
        free(graph.offsets);
        free(graph.adj);
        fail("CSR allocation failed");
    }

    for (long index = 0; index < unique_count; ++index) {
        ++graph.offsets[unique_edges[index].from + 1];
    }
    for (int vertex = 1; vertex <= n; ++vertex) {
        graph.offsets[vertex] += graph.offsets[vertex - 1];
    }
    long *positions = malloc((size_t)n * sizeof(*positions));
    if (positions == NULL) {
        free(unique_edges);
        free(graph.offsets);
        free(graph.adj);
        fail("CSR position allocation failed");
    }
    memcpy(positions, graph.offsets, (size_t)n * sizeof(*positions));
    for (long index = 0; index < unique_count; ++index) {
        graph.adj[positions[unique_edges[index].from]++] = unique_edges[index].to;
    }
    free(positions);
    free(unique_edges);
    return graph;
}

static void free_graph(graph_t *graph) {
    free(graph->offsets);
    free(graph->adj);
    graph->offsets = NULL;
    graph->adj = NULL;
}

//This function performs a breadth-first search on the graph
//It also prints the search results
static bfs_result bfs(const graph_t *graph, int source, int *distance, int *frontier, int *next_frontier) {
    for (int vertex = 0; vertex < graph->n; ++vertex) {
        distance[vertex] = -1;
    }
    distance[source] = 0;
    frontier[0] = source;
    int frontier_size = 1;
    int reached = 1;
    int levels = 0;
    long inspected_edges = 0;
    int *level_sizes = malloc((size_t)graph->n * sizeof(*level_sizes));
    if (level_sizes == NULL) {
        fail("frontier-size allocation failed");
    }
    uint64_t start = monotonic_ns();

    while (frontier_size > 0) {
        level_sizes[levels] = frontier_size;
        int next_size = 0;
        for (int index = 0; index < frontier_size; ++index) {
            int vertex = frontier[index];
            for (long edge = graph->offsets[vertex]; edge < graph->offsets[vertex + 1]; ++edge) {
                ++inspected_edges;
                int neighbor = graph->adj[edge];
                if (distance[neighbor] == -1) {
                    distance[neighbor] = distance[vertex] + 1;
                    next_frontier[next_size++] = neighbor;
                    ++reached;
                }
            }
        }
        memcpy(frontier, next_frontier, (size_t)next_size * sizeof(*frontier));
        frontier_size = next_size;
        ++levels;
    }
    uint64_t elapsed_ns = monotonic_ns() - start;
    printf("source=%d frontier_sizes=", source);
    for (int level = 0; level < levels; ++level) {
        printf("%s%d", level == 0 ? "" : ";", level_sizes[level]);
    }
    putchar('\n');
    free(level_sizes);

    bfs_result result = {inspected_edges, levels, reached, elapsed_ns};
    return result;
}

//This chooses a source vertex for the BFS
static int choose_source(const graph_t *graph, uint32_t *state) {
    for (;;) {
        *state = *state * 1664525U + 1013904223U;
        int vertex = (int)(*state % (uint32_t)graph->n);
        if (graph->offsets[vertex] < graph->offsets[vertex + 1]) {
            return vertex;
        }
    }
}

//This compares two uint64_t values; used for sorting
static int compare_u64(const void *left, const void *right) {
    uint64_t first = *(const uint64_t *)left;
    uint64_t second = *(const uint64_t *)right;
    return first > second ? 1 : (first < second ? -1 : 0);
}

//The main function will read a graph from a file and perform BFS on it
//It also prints the results
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s graph.mtx\n", argv[0]);
        return EXIT_FAILURE;
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("reading %s\n", argv[1]);
    graph_t graph = read_matrix_market(argv[1]);
    int *distance = malloc((size_t)graph.n * sizeof(*distance));
    int *frontier = malloc((size_t)graph.n * sizeof(*frontier));
    int *next_frontier = malloc((size_t)graph.n * sizeof(*next_frontier));
    uint64_t *teps = malloc(16U * sizeof(*teps));
    if (distance == NULL || frontier == NULL || next_frontier == NULL || teps == NULL) {
        free_graph(&graph);
        free(distance);
        free(frontier);
        free(next_frontier);
        free(teps);
        fail("BFS allocation failed");
    }

    printf("graph=%s vertices=%d undirected_edges=%ld\n", argv[1], graph.n, graph.m / 2);
    printf("source, levels, reached, fraction, inspected_edges, teps\n");
    uint32_t state = 12345U;
    for (int run = 0; run < 16; ++run) {
        int source = choose_source(&graph, &state);
        bfs_result result = bfs(&graph, source, distance, frontier, next_frontier);
        uint64_t rate = result.elapsed_ns == 0 ? 0 :
                        (uint64_t)((double)result.inspected_edges * 1e9 /
                                   (double)result.elapsed_ns);
        teps[run] = rate;
        printf("%d, %d, %d, %.6f, %ld, %llu\n", source, result.levels,
               result.reached, (double)result.reached / (double)graph.n,
               result.inspected_edges, (unsigned long long)rate);
    }
    qsort(teps, 16U, sizeof(*teps), compare_u64);
    printf("teps_min=%llu teps_median=%llu teps_max=%llu\n",
           (unsigned long long)teps[0],
           (unsigned long long)teps[7],
           (unsigned long long)teps[15]);

    free_graph(&graph);
    free(distance);
    free(frontier);
    free(next_frontier);
    free(teps);
    return EXIT_SUCCESS;
}
