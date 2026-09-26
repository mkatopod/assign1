## Introduction

This assignment implements and measures five kernels in C on a Linux cluster. 
All arrays use contiguous storage. Timing uses a monotonic clock.

## Common Environment

These values are from the cluster used for the experiments:

| Item          |  Value  |
| ---           | ---     |
| Node          | `run hostname and enter result` |
| Compiler      | `run gcc --version and enter result` |
| General flags | `-Wall -Wextra -std=c11 -I.../include -lrt` |
| Optimization  | `O3`, all but one |

Build targets using:
```sh
make -f MakeFile
```

## Array Maximum

**Files:** `src/array_max_part1.c`, `include/cache_sizes.sh`

Benchmark scans integer and double arrays and reports memory bandwidth as
the array size grows from 2 KiB to 256 MiB.

```sh
sh include/cache_sizes.sh > include/cache_sizes.csv
./array_max_part1 --sweep --reps 12 > include/bandwidth.txt
```

- Repetitions: 12
- Statistic: average bandwidth in bytes per second
- Plot: `python include/plot_bandwidth.py include/bandwidth.txt`

## Exclusive Prefix Sum

**Files:** `src/prefix_sum.c`, `include/prefix_sum.sh`

The program computes an exclusive prefix sum for integer and double arrays.

```sh
sh include/prefix_sum.sh
```

- Sizes: `10^6`, `10^7`, and `10^8`
- Repetitions: 5 by default; set `REPS=12` for 12 repetitions
- Optimization levels: `O0`, `O2`, `O3`
- Statistics: average microseconds, ns/element, and checksum

## Dense Matrix Multiplication

**Files:** `src/matrix_multiply.c`, `include/matrix_multiply.sh`

The program computes $C = AB$ using flat row-major arrays and implements all
six loops: `ijk`, `ikj`, `jik`, `jki`, `kij`, and `kji`.

```sh
sh include/matrix_multiply.sh
```

- Repetitions: 3 by default
- Statistic: GFLOP/s
- Correctness: every order must report `yes`


## Merge Sort

**Files:** `src/merge_sort.c`, `include/merge_sort.sh`

The benchmark compares recursive merge sort with a temporary allocation at
each merge, merge sort with one reused temporary array, and qsort.

```sh
sh include/merge_sort.sh
```

- Sizes: `10^6`, `10^7`, and `10^8`
- Inputs: random, sorted, reverse-sorted, and all equal
- Repetitions: 1 by default
- Statistic: millions of items sorted per second
- Correctness: output is sorted and contains original elements

## Breadth-First Search

**Files:** `src/bfs.c`, `include/bfs.sh`

The BFS implementation reads Matrix Market graphs. It uses a frontier array 
and reports every frontier size, number of levels, reached fraction,
inspected edges, and TEPS.

```sh
sh include/bfs.sh
```

It creates Erdos-Renyi and RMAT graphs with $2^{20}$ vertices and
average degree 16. Runs BFS from 16 randomly selected vertices.


## Scripts
```sh
sh include/cache_sizes.sh
sh include/prefix_sum.sh
sh include/matrix_multiply.sh
sh include/merge_sort.sh
sh include/bfs.sh
```



