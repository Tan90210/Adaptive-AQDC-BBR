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
        ("Week 1 - BBRv1", args.bbr_run or latest_baseline(ROOT / "results")),
        ("Week 2 - AQDC-BBRv1", args.aqdc_run or latest_baseline(ROOT / "results", "aqdc")),
    ]
    panels = [
        ("cwnd.dat", "Congestion window (segments)", read_trace),
        ("rtt.dat", "Smoothed RTT (ms)", read_trace),
        ("queueSize.dat", "Sampled queue occupancy (packets)", read_trace),
        ("throughput.dat", "Receiver goodput (Mbps)", read_throughput),
    ]
    loaded = []
    for label, folder in runs:
        traces = {}
        print(f"{label}: {folder.resolve()}")
        for filename, ylabel, reader in panels:
            trace = reader(folder / filename)
            traces[filename] = trace
            if filename == "throughput.dat":
                print(f"  Interval-weighted active-window goodput: {mean_goodput(trace):.6f} Mbps")
            elif filename in ("rtt.dat", "queueSize.dat"):
                active = trace[(trace[:, 0] >= 0.1) & (trace[:, 0] < 120), 1]
                print(f"  {ylabel}: mean={np.mean(active):.6f}, max={np.max(active):.6f}")
                if filename == "rtt.dat":
                    print(f"  Smoothed RTT change-event P99 (not raw RTT): {np.percentile(active, 99):.6f} ms")
        loaded.append((label, folder, traces))
    separate = ROOT / "analysis/figures/comparison"
    separate.mkdir(parents=True, exist_ok=True)
    names = [
        ("Congestion Window", "cwnd.png"),
        ("Smoothed TCP RTT", "rtt.png"),
        ("Bottleneck Queue Occupancy", "queue.png"),
        ("Receiver Goodput", "throughput.png"),
    ]
    provenance = " | ".join(f"{label}: {folder.name}" for label, folder, _ in loaded)
    for (filename, ylabel, _), (title, image_name) in zip(panels, names):
        single, ax = plt.subplots(figsize=(12, 5))
        for index, (label, _, traces) in enumerate(loaded):
            trace = traces[filename]
            ax.plot(trace[:, 0], trace[:, 1], label=label, linewidth=1.1,
                    color=("tab:blue", "tab:orange")[index],
                    linestyle=("-", "--")[index], alpha=0.85)
        ax.set(xlabel="Simulation time (s)", ylabel=ylabel,
               title=f"Week 1 vs Week 2: {title}")
        ax.grid(alpha=0.25)
        ax.legend(loc="best", framealpha=0.95)
        single.text(0.5, 0.015, provenance, ha="center", fontsize=8)
        single.tight_layout(rect=(0, 0.04, 1, 1))
        destination = separate / image_name
        single.savefig(destination, dpi=200)
        plt.close(single)
        print("Saved:", destination)
    print("One deterministic scenario verifies behavior; it does not establish a general latency improvement.")


if __name__ == "__main__":
    main()
