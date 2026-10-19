"""Compare the two controllers on the shared Week 1/2 fixed experiment."""

import argparse
from pathlib import Path

from plot_baseline import ROOT, latest_baseline, plt, np, read_throughput, read_trace


def mean_goodput(trace, start=0.1, stop=120.0):
    """Interval-weighted receiver goodput, including the short final interval."""
    if abs(trace[-1, 0] - stop) > 1e-6 or trace[0, 0] <= start:
        raise ValueError("Comparison requires a complete run of the current 0.1--120 s experiment")
    intervals = np.diff(np.r_[start, trace[:, 0]])
    if (intervals <= 0).any():
        raise ValueError("Goodput sampling intervals must be positive")
    return float(np.sum(trace[:, 1] * intervals) / (stop - start))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bbr-run", type=Path)
    parser.add_argument("--aqdc-run", type=Path)
    args = parser.parse_args()
    runs = [
        ("BBRv1", args.bbr_run or latest_baseline(ROOT / "results")),
        ("AQDC-BBRv1", args.aqdc_run or latest_baseline(ROOT / "results", "aqdc")),
    ]
    fig, axes = plt.subplots(2, 2, figsize=(14, 8))
    panels = [
        ("cwnd.dat", "Congestion window (segments)", read_trace),
        ("rtt.dat", "Smoothed RTT (ms)", read_trace),
        ("queueSize.dat", "Sampled queue occupancy (packets)", read_trace),
        ("throughput.dat", "Receiver goodput (Mbps)", read_throughput),
    ]
    for label, folder in runs:
        print(f"{label}: {folder.resolve()}")
        for ax, (filename, ylabel, reader) in zip(axes.flat, panels):
            trace = reader(folder / filename)
            ax.plot(trace[:, 0], trace[:, 1], linewidth=0.8, label=label, alpha=0.8)
            ax.set(xlabel="Simulation time (s)", ylabel=ylabel)
            ax.grid(alpha=0.25)
            if filename == "throughput.dat":
                print(f"  Interval-weighted active-window goodput: {mean_goodput(trace):.6f} Mbps")
            elif filename in ("rtt.dat", "queueSize.dat"):
                active = trace[(trace[:, 0] >= 0.1) & (trace[:, 0] < 120), 1]
                print(f"  {ylabel}: mean={np.mean(active):.6f}, max={np.max(active):.6f}")
                if filename == "rtt.dat":
                    print(f"  Smoothed RTT change-event P99 (not raw RTT): {np.percentile(active, 99):.6f} ms")
    for ax in axes.flat:
        ax.legend()
    fig.suptitle("Week 2: same fixed topology, stock BBR versus original AQDC reconstruction")
    fig.tight_layout()
    output = ROOT / "analysis" / "figures" / "week2-comparison.png"
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=180)
    plt.close(fig)
    print("Saved:", output)
    print("One deterministic scenario verifies behavior; it does not establish a general latency improvement.")


if __name__ == "__main__":
    main()
