"""Stage 1: reproducible fixed-coefficient experiments, raw RTT metrics and overlays.
Build first with ./ns3 build test-runner bbr-baseline-experiment aqdc-baseline-experiment aqdc-fixed-experiment -j 4.
"""
import argparse
import csv
import hashlib
import json
import re
import subprocess
from pathlib import Path

from plot_baseline import ROOT, TRACE_FILES, np, plt, read_trace, read_throughput
from compare_baselines import mean_goodput

COEFFICIENTS = (0.5, 1.0, 1.5, 2.0)  # Predeclared ablation grid, not tuned optima.
# Two-curve presentation isolates Stage 1's change. Alpha=1.5 illustrates stronger
# compensation; it is not selected as an optimum from performance results.
PRESENTATION = {"original": "Original AQDC (alpha = 1)",
                "fixed-1.5": "Stage 1 fixed AQDC (alpha = 1.5)"}


def read_stage1_rtt(path):
    lines = path.read_text().splitlines()
    if not lines or lines[0] != "# stage1-rtt-v1 time_s raw_estimator_rtt_ms smoothed_rtt_ms":
        raise ValueError(f"Stage 1 raw RTT schema required: {path}")
    metadata = dict(re.findall(r"(\w+)=([^ ]+)", lines[1]))
    data = np.atleast_2d(np.loadtxt(path))
    if data.shape[1] != 3 or not np.isfinite(data).all():
        raise ValueError(f"Expected finite time/raw/smoothed columns: {path}")
    if (data[:, 0] < 0).any() or (data[:, 1:] <= 0).any() or (np.diff(data[:, 0]) < 0).any():
        raise ValueError(f"Invalid RTT sample or timestamp: {path}")
    return data, metadata


def queue_time_mean(trace, start, stop):
    """Integrate the sampled queue's piecewise-constant reconstruction."""
    if trace[0, 0] > start or trace[-1, 0] < stop:
        raise ValueError("Queue trace does not cover the measurement window")
    inside = trace[(trace[:, 0] > start) & (trace[:, 0] < stop)]
    initial = trace[np.searchsorted(trace[:, 0], start, side="right") - 1, 1]
    times = np.r_[start, inside[:, 0], stop]
    values = np.r_[initial, inside[:, 1]]
    return float(np.dot(np.diff(times), values) / (stop - start))


def validate_events(log):
    episodes, pending = [], None
    for line in log.splitlines():
        if not line.startswith(("AQDC entry ", "AQDC complete ")):
            continue
        event = dict((key, float(value)) for key, value in re.findall(r"(\w+)=([0-9.eE+\-]+)", line))
        if line.startswith("AQDC entry "):
            if pending is not None:
                raise ValueError("Compensation entered twice without completion")
            pending = event
        else:
            if pending is None:
                raise ValueError("Completion without entry")
            if event["inflight_bytes"] > event["target_bytes"]:
                raise ValueError("Completion above target")
            for key in ("debt_bytes", "target_bytes", "coefficient"):
                if event[key] != pending[key]:
                    raise ValueError("Episode snapshot changed")
            if event["time_s"] < pending["time_s"]:
                raise ValueError("Negative episode duration")
            episodes.append(dict(pending, completion_s=event["time_s"],
                                 completion_inflight_bytes=event["inflight_bytes"],
                                 duration_ms=1000 * (event["time_s"] - pending["time_s"])))
            pending = None
    summary = re.search(r"AQDC summary debt_increases=(\d+) entries=(\d+) completions=(\d+)", log)
    if summary:
        _, entries, completions = map(int, summary.groups())
        if entries != len(episodes) + int(pending is not None) or completions != len(episodes):
            raise ValueError("Parsed episode counts disagree with controller summary")
    return episodes, pending


def traces_identical(first, second):
    return all((first / name).read_bytes() == (second / name).read_bytes()
               for name in TRACE_FILES if name != "rtt.dat") and np.array_equal(
                   read_stage1_rtt(first / "rtt.dat")[0], read_stage1_rtt(second / "rtt.dat")[0])


def write_csv(path, rows):
    with path.open("w", newline="") as out:
        writer = csv.DictWriter(out, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def analyse(manifest_path, publish=False):
    manifest = json.loads(manifest_path.read_text())
    output = manifest_path.parent
    figures = ROOT / "analysis/figures/stage1"
    start, stop = 0.1, float(manifest["duration_s"])
    rows, event_rows, loaded = [], [], []
    for run in manifest["runs"]:
        folder = ROOT / run["results"]
        if set(p.name for p in folder.iterdir()) != set(TRACE_FILES):
            raise ValueError(f"Run must contain exactly four data files: {folder}")
        rtt, meta = read_stage1_rtt(folder / "rtt.dat")
        if (float(meta["stop_s"]) != stop or float(meta["start_s"]) != start or
                meta["controller"] != run["controller"] or
                float(meta["coefficient"]) != run["coefficient"]):
            raise ValueError("Incompatible controller, coefficient or measurement window")
        raw = rtt[(rtt[:, 0] >= start) & (rtt[:, 0] < stop), 1]
        if not len(raw):
            raise ValueError("No active-window raw RTT samples")
        cwnd = read_trace(folder / "cwnd.dat")
        queue = read_trace(folder / "queueSize.dat")
        goodput = read_throughput(folder / "throughput.dat")
        console = (output / run["console_log"]).read_text()
        if run["controller"] != "ns3::TcpBbr" and "AQDC summary " not in console:
            raise ValueError("Missing AQDC controller summary")
        episodes, pending = validate_events(console)
        entries = len(episodes) + int(pending is not None)
        active_queue = queue[(queue[:, 0] >= start) & (queue[:, 0] < stop), 1]
        rows.append(dict(stage="stage1", label=run["label"], coefficient=run["coefficient"],
                         results=run["results"], duration_s=stop, raw_rtt_samples=len(raw),
                         raw_rtt_mean_ms=float(np.mean(raw)), raw_rtt_median_ms=float(np.median(raw)),
                         raw_rtt_p95_ms=float(np.percentile(raw, 95)),
                         raw_rtt_p99_ms=float(np.percentile(raw, 99)),
                         goodput_mbps=mean_goodput(goodput, start, stop),
                         sampled_queue_time_mean_packets=queue_time_mean(queue, start, stop),
                         sampled_queue_max_packets=float(np.max(active_queue)),
                         compensation_entries=entries, compensation_completions=len(episodes),
                         unfinished_episodes=int(pending is not None),
                         compensation_total_ms=sum(e["duration_ms"] for e in episodes)))
        for number, event in enumerate(episodes, 1):
            event_rows.append(dict(label=run["label"], episode=number, **event))
        if pending:
            # An unfinished episode is reported, not silently called successful.
            event_rows.append(dict(label=run["label"], episode=entries, **pending,
                                   completion_s="", completion_inflight_bytes="", duration_ms=""))
        loaded.append((run["label"], cwnd, rtt, queue, goodput, raw, run["key"]))
    original = next(ROOT / x["results"] for x in manifest["runs"] if x["key"] == "original")
    alpha_one = next(ROOT / x["results"] for x in manifest["runs"] if x["key"] == "fixed-1")
    if not traces_identical(original, alpha_one):
        raise ValueError("Fixed alpha=1 does not reproduce original AQDC")
    manifest["alpha_one_matches_original"] = True
    manifest["analysis_window_s"] = [start, stop]
    manifest["analysis_script_sha256"] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    manifest["network"] = dict(access_mbps=100, bottleneck_mbps=10, propagation_rtt_ms=50,
                               bottleneck_fifo_bytes=62500, device_queue_packets=1, mss_bytes=1448,
                               queue_goodput_sample_interval_s=0.2, receiver_drain_s=1)
    manifest["percentile_method"] = "numpy linear; unweighted valid TCP estimator-input samples, not per-packet RTT"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    write_csv(output / "stage1-fixed-coefficient-metrics.csv", rows)
    if event_rows:
        write_csv(output / "stage1-compensation-events.csv", event_rows)
    if publish:
        figures.mkdir(parents=True, exist_ok=True)
        panels = ((1, "Congestion window (segments)", "cwnd"),
                  (2, "Raw RTT estimator sample (ms)", "rtt"),
                  (3, "Sampled bottleneck queue (packets)", "queue"),
                  (4, "Receiver application goodput (Mbps)", "throughput"))
        presentation = [item for item in loaded if item[-1] in PRESENTATION]
        if {item[-1] for item in presentation} != set(PRESENTATION):
            raise ValueError("Presentation requires original AQDC and fixed alpha=1.5")
        for index, ylabel, name in panels:
            fig, ax = plt.subplots(figsize=(12, 5))
            for i, item in enumerate(presentation):
                data = item[index]
                ax.plot(data[:, 0], data[:, 1], label=PRESENTATION[item[-1]], linewidth=1.0,
                        alpha=0.85, color=("tab:blue", "tab:orange")[i],
                        linestyle=("-", "--")[i])
            ax.set(xlabel="Simulation time (s)", ylabel=ylabel, xlim=(0, stop),
                   title=f"Stage 1 — original AQDC vs fixed alpha=1.5: {name} ({stop:g} s)")
            ax.grid(alpha=0.25)
            ax.legend(fontsize=9, framealpha=0.95)
            fig.text(0.5, 0.015, f"Source: {manifest_path.relative_to(ROOT)}; seed=1, run=1",
                     ha="center", fontsize=7)
            fig.tight_layout(rect=(0, 0.04, 1, 1))
            fig.savefig(figures / f"stage1-{name}.png", dpi=180)
            plt.close(fig)
        write_csv(ROOT / "analysis/stage1/stage1-metrics.csv", rows)
        (ROOT / "analysis/stage1/stage1-current-run.json").write_text(json.dumps(
            {"manifest": str(manifest_path.relative_to(ROOT)), "duration_s": stop,
             "plotted_controllers": PRESENTATION,
             "selection_note": "alpha=1.5 illustrates stronger compensation, not an optimum; metrics retain full sweep",
             "graphs": [f"analysis/figures/stage1/stage1-{name}.png" for _, _, name in panels]},
            indent=2) + "\n")
    print("Stage 1 alpha=1 trace equivalence: PASS")
    for row in rows:
        print(f"{row['label']}: goodput={row['goodput_mbps']:.6f} Mbps, raw P99={row['raw_rtt_p99_ms']:.6f} ms, episodes={row['compensation_entries']}")
    print("Metrics:", output)
    if publish:
        print("Four presentation graphs:", figures)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--duration", type=float, default=120.0)
    parser.add_argument("--analyse", type=Path, help="Reanalyse this saved Stage 1 manifest without new runs")
    parser.add_argument("--publish", action="store_true",
                        help="Publish exactly four Stage 1 graphs and the main metrics CSV")
    args = parser.parse_args()
    if args.analyse:
        analyse(args.analyse.resolve(), publish=args.publish)
        return
    if not np.isfinite(args.duration) or args.duration <= 0.3:
        parser.error("duration must be finite and greater than 0.3 seconds")
    base = ROOT / "analysis/stage1"
    base.mkdir(parents=True, exist_ok=True)
    for version in range(1, 100000):
        output = base / f"stage1-duration-{args.duration:g}s-v{version}"
        try:
            output.mkdir()
            break
        except FileExistsError:
            pass
    definitions = [("bbr", "BBRv1", "bbr-baseline-experiment", "ns3::TcpBbr", 1.0),
                   ("original", "Original AQDC", "aqdc-baseline-experiment", "ns3::TcpAqdcBbr", 1.0)]
    definitions += [(f"fixed-{a:g}", f"Fixed alpha={a:g}", "aqdc-fixed-experiment", "ns3::TcpAqdcFixed", a) for a in COEFFICIENTS]
    sources = ("scratch/baseline-experiment.h", "src/internet/model/tcp-aqdc-bbr.cc",
               "src/internet/model/tcp-aqdc-fixed.cc", "src/internet/model/tcp-socket-base.cc",
               "analysis/stage1_fixed_coefficients.py")
    manifest = dict(stage="stage1", duration_s=args.duration, seed=1, run=1, coefficients=list(COEFFICIENTS),
                    purpose="Fixed-coefficient ablation; no adaptation or tuned optimum",
                    git_head=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    git_status=subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
                    source_sha256={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources}, runs=[])
    path = output / "stage1-run-manifest.json"
    for key, label, executable, controller, coefficient in definitions:
        command = f"{executable} --stage1=1 --duration={args.duration:g} --diagnostics=1"
        if executable == "aqdc-fixed-experiment":
            command += f" --coefficient={coefficient:g}"
        print("Running", command, flush=True)
        result = subprocess.run([str(ROOT / "ns3"), "run", command, "--no-build"], cwd=ROOT,
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log_name = f"stage1-{key}.console.log"
        (output / log_name).write_text(result.stdout)
        if result.returncode:
            raise RuntimeError(f"Run failed; see {output / log_name}")
        match = re.search(r"^Results: \"?([^\n\"]+)", result.stdout, re.MULTILINE)
        if not match:
            raise ValueError(f"Missing results location; see {output / log_name}")
        folder = Path(match.group(1).strip())
        folder = folder if folder.is_absolute() else ROOT / folder
        manifest["runs"].append(dict(key=key, label=label, coefficient=coefficient, controller=controller,
                                     command=command, results=str(folder.relative_to(ROOT)), console_log=log_name))
        path.write_text(json.dumps(manifest, indent=2) + "\n")
    analyse(path, publish=True)


if __name__ == "__main__":
    main()
