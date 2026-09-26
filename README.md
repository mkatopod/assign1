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

Example of Output
| Array | n | Reps | Average (us) | Median (us) | Min (us) | Max (us) | Bandwidth (MB/s) | ns/element | Result |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| int A sorted | 1000000 | 12 | 215.385 | 212.827 | 212.629 | 232.161 | 17711.063 | 0.215 | 131070 |
| int B sorted | 1000000 | 12 | 217.059 | 212.912 | 212.698 | 251.940 | 17574.472 | 0.217 | 131070 |


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

Example of Output 
| optimization | type | n | reps | avg_us | ns_per_element | checksum |
| O0 | int | 1000000 | 5 | 2437.093 | 2.437093 | 499999 |
| O0 | double | 1000000 | 5 | 2539.302 | 2.539302 | 499999.0 |


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

Example of Output
| M | K | N | order | reps | avg_us | gflops | correct |
| 256 | 256 | 256 | ijk | 3 | 17808.338 | 1.884198 | yes | 
| 256 | 256 | 256 | ikj | 3 | 2576.756 | 13.021967 | yes |
| 256 | 256 | 256 | jik | 3 | 17971.383 | 1.867104 | yes | 


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

Example of Output
| n | input | method | repetitions | avg_us | rate_mitems_per_s | sorted_and_preserved |
| 1000000 | random | merge_per_call | 1 | 115119.587 | 8.687 | yes |
| 1000000 | random | merge_reuse | 1 | 86747.895 | 11.528 | yes |


## Breadth-First Search

**Files:** `src/bfs.c`, `include/bfs.sh`

The BFS implementation reads Matrix Market graphs into CSR storage. It uses a 
frontier array and reports every frontier size, number of levels, reached fraction,
inspected edges, and TEPS.

```sh
sh include/bfs.sh
```

It creates Erdos-Renyi and RMAT graphs with $2^{20}$ vertices and
average degree 16. Runs BFS from 16 randomly selected vertices.

Results are written to `bfs_graphs/`. 

Example of Output [From erdos_renyi]
| source | levels | reached | fraction | inspected_edges | teps | 
| source=597060 frontier_sizes=1;18;296;4705;72477;650221;320836;22 597060 | 8 | 1048576 | 1.000000 | 16777110 | 122463305 | 
| source=817875 frontier_sizes=1;26;393;6380;96437;728157;217180;2 817875 | 8 | 1048576 | 1.000000 | 16777110 | 129909583 |


## Scripts Used
```sh
sh include/cache_sizes.sh
sh include/prefix_sum.sh
sh include/matrix_multiply.sh
sh include/merge_sort.sh
sh include/bfs.sh
```

