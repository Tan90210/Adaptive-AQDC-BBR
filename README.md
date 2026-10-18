# Adaptive-AQDC-BBR

Baseline experiments for adaptive queue-debt compensation in ns-3 **3.46.1**. The current implementation runs stock BBRv1; original AQDC and adaptive compensation are the next phases.

Work from the Ubuntu WSL checkout:

```bash
cd ~/Adaptive-AQDC-BBR
```

## Configure, build, and run

```bash
./ns3 configure --enable-modules=applications,flow-monitor,internet,point-to-point,traffic-control --disable-examples --disable-tests
./ns3 build aqdc-experiment -j 4
./ns3 run aqdc-experiment
```

Each run creates the next free `results/bbr-baseline-v<number>/` directory and preserves previous baselines. It contains exactly:

| File | Columns |
| --- | --- |
| `cwnd.dat` | Simulation time (s), congestion window (segments) |
| `rtt.dat` | Simulation time (s), smoothed TCP RTT (ms) |
| `queueSize.dat` | Simulation time (s), forward bottleneck FIFO occupancy (packets) |
| `throughput.dat` | Simulation time with `s` suffix, receiver goodput, `Mbps` |

The single experiment uses one long-lived TCP flow, 100 Mbps access links, a fixed 10 Mbps bottleneck, 50 ms round-trip propagation delay, and a 62,500-byte (1 BDP) FIFO. Device queues hold at most one additional packet each. The sender runs from 0.1 to 120 seconds, with one additional second of receiver drain time. Queue and throughput sampling interval is 0.2 seconds.

## Plot results

Create a local Python environment and install the analysis dependencies:

```bash
python3 -m venv --without-pip .venv
python3 -m pip --python .venv install -r analysis/requirements.txt
.venv/bin/python analysis/plot_baseline.py
```

The environment commands use the system's existing pip and work without the Ubuntu ensurepip package. The plotter selects the highest numbered baseline with all four nonempty files. It saves four graphs to `analysis/figures/` and prints the selected run. It works from any current directory. To inspect a particular version:

```bash
.venv/bin/python analysis/plot_baseline.py --run-dir results/bbr-baseline-v1
```

`rtt.dat` records smoothed RTT change events. Its percentiles are descriptive estimator statistics, not raw RTT tail latency. Queue statistics are sampled and can miss short spikes. The old v1 traces came from the earlier experiment and should not be assumed to have the current settings or throughput semantics. Use current runs for subsequent controlled comparisons.

## Checks and next phases

```bash
.venv/bin/python -m unittest discover -s analysis -p 'test_*.py'
```

The original AQDC and adaptive controllers are not implemented yet. Keep stock BBR available and validate debt accounting and ProbeRTT transitions when adding those controllers. Raw RTT observations will be needed for defensible P95/P99 latency comparisons; the current four-file baseline retains the original smoothed RTT format.

Build caches, binaries, lock files, `.venv/`, and new baseline runs are local generated artifacts and are ignored. The existing tracked v1 reference remains tracked; selected future experiment results can be added explicitly with `git add -f`. The original stale CMake cache was preserved outside the checkout during migration. Do not commit generated build state. No temporary `/tmp` build cache or `--no-build` workaround is required for normal use.
