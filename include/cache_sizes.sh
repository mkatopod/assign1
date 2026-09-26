#!/bin/bash
#SBATCH --time=00:30:00
#SBATCH --partition=Centaurus
set -eu

repo_dir=${SLURM_SUBMIT_DIR:-"$PWD"}
if [[ ${repo_dir##*/} == include ]]; then
  repo_dir=$(dirname -- "$repo_dir")
fi
cd "$repo_dir"

printf 'level,bytes\n'
lscpu | awk '
  /L1d cache:/ { gsub(/K/, "", $3); print "L1," $3 * 1024 }
  /L2 cache:/  { gsub(/K/, "", $3); print "L2," $3 * 1024 }
  /L3 cache:/  { gsub(/M/, "", $3); print "L3," $3 * 1024 * 1024 }
'

gcc -O3 -Wall -Wextra -std=c11 -Iinclude src/array_max_part1.c -o array_max_part1 -lrt
echo "array_bytes,bandwidth_bytes_per_s" > bandwidth.txt

sizes=(2048 4096 8192 16384 32768 65536 131072 262144 \
       524288 1048576 2097152 4194304 8388608 16777216 \
       33554432 67108864 134217728 268435456)

for n in "${sizes[@]}"; do
    bw=$(./array_max_part1 --sizes $n --orders random --reps 12 | awk -F'rate=' '/rate=/{val=$2} END{print val}' | awk '{print $1 * 1024 * 1024}')
    if [ -n "$bw" ]; then
        echo "$n,$bw" >> bandwidth.txt
    fi
done
