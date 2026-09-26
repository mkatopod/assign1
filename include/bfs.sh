#!/bin/sh
#SBATCH --job-name=csr-bfs
#SBATCH --partition=Centaurus
#SBATCH --time=02:00:00
#SBATCH --mem=32G

set -eu

submit_dir=${SLURM_SUBMIT_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
if [ -f "$submit_dir/MakeFile" ] || [ -f "$submit_dir/Makefile" ]; then
    project_dir=$submit_dir
else
    project_dir=$(CDPATH= cd -- "$submit_dir/.." && pwd)
fi

cd "$project_dir"

if [ -f "$project_dir/MakeFile" ]; then
    makefile=$project_dir/MakeFile
else
    makefile=$project_dir/Makefile
fi

vertices=${BFS_VERTICES:-1048576}
average_degree=${BFS_DEGREE:-16}
edges=$((vertices * average_degree / 2))
output_dir=${BFS_OUTPUT_DIR:-bfs_graphs}

mkdir -p "$output_dir"

make -f "$makefile" bfs OPT_LEVEL="${OPT_LEVEL:-O3}"

for model in erdos rmat; do
    if [ "$model" = erdos ]; then
        graph_name=erdos_renyi
    else
        graph_name=rmat
    fi

    printf 'Generating %s graph...\n' "$graph_name"

    python3 - "$vertices" "$edges" "$output_dir/${graph_name}.mtx" "$model" "${BFS_SEED:-4145}" <<'PY'
import random
import sys


n = int(sys.argv[1])
m = int(sys.argv[2])
path = sys.argv[3]
model = sys.argv[4]
seed = int(sys.argv[5]) if len(sys.argv) > 5 else 4145
ng = random.Random(seed)
scale = (n - 1).bit_length()

with open(path, "w", buffering=1024 * 1024) as output:
    output.write("%%MatrixMarket matrix coordinate pattern symmetric\n")
    output.write("%% generated undirected graph\n")
    output.write(f"{n} {n} {m}\n")
    written = 0
    while written < m:
        if model == "erdos":
            left = ng.randrange(n)
            right = ng.randrange(n)
        else:
            row = 0
            column = 0
            for bit in range(scale):
                draw = ng.random()
                if draw < 0.57:
                    pass
                elif draw < 0.77:
                    row |= 1 << bit
                elif draw < 0.97:
                    column |= 1 << bit
                else:
                    row |= 1 << bit
                    column |= 1 << bit
            left = row % n
            right = column % n
        if left == right:
            continue
        output.write(f"{left + 1} {right + 1}\n")
        written += 1
PY
    printf 'Generated %s.mtx; starting BFS...\n' "$graph_name"
done

for graph in "$output_dir/erdos_renyi.mtx" "$output_dir/rmat.mtx"; do
    name=$(basename "$graph" .mtx)
    report="$output_dir/${name}.out"
    temporary_report="$report.tmp"
    if ! ./bfs "$graph" > "$temporary_report" 2>&1; then
        mv "$temporary_report" "$report"
        printf 'BFS failed for %s; see %s\n' "$graph" "$report" >&2
        exit 1
    fi
    mv "$temporary_report" "$report"
    printf '%s: ' "$name"
    awk '/teps_min=/{print; found=1} END{if (!found) exit 1}' "$report"
done

printf 'Graph files and BFS reports are in %s\n' "$output_dir"