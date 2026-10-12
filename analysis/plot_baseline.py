
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt

ROOT = Path.home() / "ns-3-aqdc"

# Most recent baseline experiment containing all four traces
candidates = sorted(
    p for p in (ROOT / "results/bbr-baseline").iterdir()
    if p.is_dir()
    and all((p / f).exists() for f in [
        "cwnd.dat", "rtt.dat",
        "queueSize.dat", "throughput.dat"
    ])
)

if not candidates:
    raise FileNotFoundError("No complete baseline run found")

run_dir = max(candidates, key=lambda p: p.stat().st_mtime)
output = ROOT / "analysis" / "figures"
output.mkdir(parents=True, exist_ok=True)

def read_two_columns(filename):
    return np.atleast_2d(np.loadtxt(run_dir / filename))

cwnd = read_two_columns("cwnd.dat")
rtt = read_two_columns("rtt.dat")
queue = read_two_columns("queueSize.dat")

# Throughput data contains suffixes such as "0.2s 10Mbps"
throughput = []
with open(run_dir / "throughput.dat") as f:
    for line in f:
        parts = line.split()
        if len(parts) >= 3:
            throughput.append((
                float(parts[0].removesuffix("s")),
                float(parts[1])
            ))

throughput = np.array(throughput)

def plot_trace(data, ylabel, title, filename):
    plt.figure(figsize=(10, 4))
    plt.plot(data[:, 0], data[:, 1], linewidth=1)
    plt.xlabel("Simulation time (s)")
    plt.ylabel(ylabel)
    plt.title(title)
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(output / filename, dpi=200)
    plt.close()

plot_trace(cwnd, "Congestion window (segments)",
           "BBRv1 Congestion Window", "cwnd.png")

plot_trace(rtt, "Smoothed RTT (ms)",
           "BBRv1 TCP RTT Estimate", "rtt.png")

plot_trace(queue, "Queue occupancy (reported units)",
           "BBRv1 Bottleneck Queue", "queue.png")

plot_trace(throughput, "Throughput (Mbps)",
           "BBRv1 Throughput", "throughput.png")

print("Baseline run:", run_dir)
print("Graphs saved to:", output)
print()
print("TCP RTT estimator statistics:")
print("Mean:", round(np.mean(rtt[:, 1]), 2), "ms")
print("P95:", round(np.percentile(rtt[:, 1], 95), 2), "ms")
print("P99:", round(np.percentile(rtt[:, 1], 99), 2), "ms")
print()
print("Average reported throughput:",
      round(np.mean(throughput[:, 1]), 2), "Mbps")
print("Average sampled queue occupancy:",
      round(np.mean(queue[:, 1]), 2))
print("Maximum sampled queue occupancy:",
      np.max(queue[:, 1]))
