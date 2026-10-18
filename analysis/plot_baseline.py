"""Plot the latest complete four-file BBR baseline from this checkout."""

import argparse
from pathlib import Path
import re

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
TRACE_FILES = ("cwnd.dat", "rtt.dat", "queueSize.dat", "throughput.dat")


def latest_baseline(results_dir):
    candidates = []
    if results_dir.is_dir():
        for path in results_dir.iterdir():
            match = re.fullmatch(r"bbr-baseline-v([1-9][0-9]*)", path.name)
            if match and path.is_dir() and all(
                (path / name).is_file() and (path / name).stat().st_size > 0
                for name in TRACE_FILES
            ):
                candidates.append((int(match.group(1)), path))
    if not candidates:
        raise FileNotFoundError(f"No complete bbr-baseline-v<number> run in {results_dir}")
    return max(candidates, key=lambda item: item[0])[1]


def validate_trace(data, filename):
    if data.ndim != 2 or data.shape[1] != 2 or not len(data):
        raise ValueError(f"Expected two nonempty columns in {filename}")
    if not np.isfinite(data).all() or (data < 0).any():
        raise ValueError(f"Nonfinite or negative values in {filename}")
    if (np.diff(data[:, 0]) < 0).any():
        raise ValueError(f"Timestamps are out of order in {filename}")
    return data


def read_trace(path):
    return validate_trace(np.atleast_2d(np.loadtxt(path)), path)


def read_throughput(path):
    rows = []
    for number, line in enumerate(path.read_text().splitlines(), 1):
        if not line.strip():
            continue
        fields = line.split()
        if len(fields) != 3 or not fields[0].endswith("s") or fields[2] != "Mbps":
            raise ValueError(f"Invalid throughput format at {path}:{number}")
        rows.append((float(fields[0][:-1]), float(fields[1])))
    return validate_trace(np.asarray(rows), path)


def plot_trace(data, ylabel, title, destination):
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(data[:, 0], data[:, 1], linewidth=1)
    ax.set(xlabel="Simulation time (s)", ylabel=ylabel, title=title)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(destination, dpi=200)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-dir", type=Path, help="Choose a specific baseline folder")
    args = parser.parse_args()
    run_dir = args.run_dir.resolve() if args.run_dir else latest_baseline(ROOT / "results")
    cwnd = read_trace(run_dir / "cwnd.dat")
    rtt = read_trace(run_dir / "rtt.dat")
    queue = read_trace(run_dir / "queueSize.dat")
    throughput = read_throughput(run_dir / "throughput.dat")
    output = ROOT / "analysis" / "figures"
    output.mkdir(parents=True, exist_ok=True)
    plots = [
        (cwnd, "Congestion window (segments)", "BBRv1 Congestion Window", "cwnd.png"),
        (rtt, "Smoothed RTT (ms)", "BBRv1 TCP RTT Estimate", "rtt.png"),
        (queue, "Queue occupancy (packets)", "BBRv1 Bottleneck Queue", "queue.png"),
        (throughput, "Receiver goodput (Mbps)", "BBRv1 Receiver Goodput", "throughput.png"),
    ]
    for data, ylabel, title, filename in plots:
        plot_trace(data, ylabel, f"{title} - {run_dir.name}", output / filename)
    print("Baseline run:", run_dir)
    print("Graphs saved to:", output)
    print("Smoothed RTT change-event statistics (not raw RTT percentiles):")
    print(f"Mean: {np.mean(rtt[:, 1]):.2f} ms")
    print(f"P95: {np.percentile(rtt[:, 1], 95):.2f} ms")
    print(f"P99: {np.percentile(rtt[:, 1], 99):.2f} ms")
    print(f"Average sampled receiver goodput: {np.mean(throughput[:, 1]):.2f} Mbps")
    print(f"Average sampled queue occupancy: {np.mean(queue[:, 1]):.2f} packets")
    print(f"Maximum sampled queue occupancy: {np.max(queue[:, 1]):.0f} packets")


if __name__ == "__main__":
    main()
