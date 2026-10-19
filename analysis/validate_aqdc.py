"""Run AQDC and verify its exact lifecycle diagnostics without extra result files."""

import re
import subprocess

from plot_baseline import ROOT, TRACE_FILES, latest_baseline, read_throughput, read_trace


def main():
    result = subprocess.run(
        ["./ns3", "run", "aqdc-baseline-experiment --diagnostics=1"],
        cwd=ROOT, text=True, capture_output=True, check=True,
    )
    print(result.stdout, end="")
    pending = None
    completed = 0
    for event in re.finditer(
        r"AQDC (entry|complete) time_s=([\d.eE+-]+) debt_bytes=([\d.eE+-]+) "
        r"target_bytes=(\d+) inflight_bytes=(\d+)", result.stdout
    ):
        kind, time, debt, target, inflight = event.groups()
        time, debt, target, inflight = float(time), float(debt), int(target), int(inflight)
        assert debt >= 0 and target >= 4 * 1448
        if kind == "entry":
            assert pending is None, "Nested compensation episode"
            pending = (time, debt, target)
        else:
            assert pending is not None, "Completion without entry"
            assert time >= pending[0] and debt == pending[1] and target == pending[2]
            assert inflight <= target, "Actual inflight must reach target before exit"
            pending = None
            completed += 1
    summary = re.search(r"AQDC summary debt_increases=(\d+) entries=(\d+) completions=(\d+)", result.stdout)
    assert summary, "No controller summary"
    increases, entries, completions = map(int, summary.groups())
    assert increases > 0 and entries > 0, "The scenario must exercise debt and compensation"
    assert completed == completions and entries == completions and pending is None
    folder = latest_baseline(ROOT / "results", "aqdc")
    assert {path.name for path in folder.iterdir()} == set(TRACE_FILES)
    for name in TRACE_FILES:
        (read_throughput if name == "throughput.dat" else read_trace)(folder / name)
    print(f"PASS: {completed} complete compensation episodes; actual inflight met every target.")
    print("PASS: exactly four populated, valid .dat files in", folder)


if __name__ == "__main__":
    main()
