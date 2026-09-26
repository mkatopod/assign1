import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt

#Reads cache sizes
def read_cache_sizes(path):
    with open(path, newline="") as handle:
        return {row["level"]: float(row["bytes"]) for row in csv.DictReader(handle)}

#Reads bandwidth data
def read_bandwidth(path):
    with open(path, newline="") as handle:
        rows = list(csv.DictReader(handle))

    if not rows:
        raise ValueError(f"no benchmark rows found in {path}")

    if "bandwidth_bytes_per_s" in rows[0]:
        return (
            [int(row["array_bytes"]) for row in rows],
            [float(row["bandwidth_bytes_per_s"]) for row in rows],
        )

    if "bandwidth" in rows[0]:
        return (
            [int(row["array_bytes"]) for row in rows],
            [float(row["bandwidth"]) for row in rows],
        )

    raise ValueError(
        "benchmark CSV must contain bandwidth_bytes_per_s or bandwidth column"
    )

#In main, it plots memory bandwidth data
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_file")
    parser.add_argument(
        "--cache-csv", default=Path(__file__).with_name("cache_sizes.csv")
    )
    parser.add_argument("--output", default="bandwidth.png")
    args = parser.parse_args()

    array_bytes, bandwidth_bytes_per_s = read_bandwidth(args.csv_file)
    bandwidth = [value / (1024 ** 3) for value in bandwidth_bytes_per_s]
    cache_sizes = read_cache_sizes(args.cache_csv)

    plt.figure(figsize=(8, 5))
    plt.plot(array_bytes, bandwidth, marker="o", markersize=3)
    for level in ("L1", "L2", "L3"):
        if level in cache_sizes:
            size = cache_sizes[level]
            label = f"{level} ({size / 1024:.0f} KiB)" if size < 1024 ** 2 else f"{level} ({size / 1024 ** 2:.0f} MiB)"
            plt.axvline(size, linestyle="--", label=label)
    plt.xscale("log", base=2)
    plt.xlabel("Array size (bytes)")
    plt.ylabel("Read bandwidth (GiB/s)")
    plt.title("Double branchless maximum: memory bandwidth vs. array size")
    plt.grid(True, which="both", alpha=0.25)
    plt.legend()
    plt.tight_layout()
    plt.savefig(args.output, dpi=160)

#If this script is run directly, it should execute the main function
if __name__ == "__main__":
    main()