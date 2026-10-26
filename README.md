# Adaptive-AQDC-BBR

Course project using ns-3 **3.46.1**. Week 1 provides stock BBRv1; Week 2 reconstructs original AQDC-BBRv1. Adaptive compensation remains Week 3 work. The implementation and its interpretation choices are explained step by step in [doc/aqdc-week2.md](doc/aqdc-week2.md).

## Configure and build in Ubuntu WSL

```bash
cd ~/Adaptive-AQDC-BBR
./ns3 configure --enable-tests --disable-examples --enable-modules=applications,flow-monitor,internet,point-to-point,traffic-control
./ns3 build test-runner bbr-baseline-experiment aqdc-baseline-experiment -j 4
```

## Run the same fixed experiment

```bash
./ns3 run bbr-baseline-experiment
./ns3 run aqdc-baseline-experiment
```

The executables share one topology and measurement harness. Each run creates the next free `results/bbr-baseline-vN/` or `results/aqdc-baseline-vN/` folder, preserving earlier results. Each folder contains only:

| File | Columns |
| --- | --- |
| `cwnd.dat` | Simulation time (s), congestion window (segments) |
| `rtt.dat` | Simulation time (s), smoothed TCP RTT (ms) |
| `queueSize.dat` | Simulation time (s), forward bottleneck FIFO occupancy (packets) |
| `throughput.dat` | Simulation time with `s` suffix, receiver goodput, `Mbps` |

Settings: one TCP flow, 100 Mbps access links, a fixed 10 Mbps bottleneck, 50 ms round-trip propagation delay, and a 62,500-byte (1 BDP) FIFO. Device queues hold at most one additional packet. The sender runs from 0.1 to 120 seconds, followed by one second of receiver drain time. Queue and goodput sampling interval is 0.2 seconds.

## Validate Week 2

```bash
./ns3 run "test-runner --suite=tcp-aqdc --verbose"
./ns3 run "test-runner --suite=tcp-bbr-test --verbose"
```

Print optional compensation diagnostics to the console:

```bash
./ns3 run "aqdc-baseline-experiment --diagnostics=1"
```

## Python analysis

```bash
python3 -m venv --without-pip .venv
python3 -m pip --python .venv install -r analysis/requirements.txt
.venv/bin/python analysis/plot_baseline.py
.venv/bin/python analysis/plot_baseline.py --algorithm=aqdc
.venv/bin/python analysis/compare_baselines.py
.venv/bin/python analysis/validate_aqdc.py
.venv/bin/python -m unittest discover -s analysis -p 'test_*.py'
```

The plotter chooses the latest numbered run of the selected controller. Stock BBR figures go to `analysis/figures/bbrv1/`; AQDC figures go to `analysis/figures/aqdc/`. The comparison creates four overlays in `analysis/figures/comparison/` and prints numerical statistics. Use `--run-dir` with the plotter, or `--bbr-run` and `--aqdc-run` with the comparison, to select exact runs. The validator executes AQDC and checks its exact compensation events and four-file output.

`rtt.dat` contains smoothed RTT change events, so its percentiles must not be presented as raw RTT tail latency. Queue samples can miss brief spikes. Old v1 data came from an earlier experiment and must not be assumed to share current settings. Week 2 validates reproduction behavior, not the paper's reported percentages; raw RTT collection is available in final-phase Fixed-coefficient AQDC; adaptive compensation remains future work.

Build caches, binaries, lock files, `.venv/`, and new baseline runs are ignored. The v1 reference and selected datasets referenced by the saved validation manifests are versioned; other generated runs remain ignored. Selected experiment results can be added explicitly with `git add -f`. Normal builds use this checkout's persistent CMake cache; no temporary-cache workaround is needed.

The comparison script also saves four separate Week 1/Week 2 overlay graphs in `analysis/figures/comparison/`: `cwnd.png`, `rtt.png`, `queue.png`, and `throughput.png`. Blue solid lines show Week 1 BBRv1; orange dashed lines show Week 2 AQDC-BBRv1. Each figure identifies its source runs.


## Final phase — Fixed-coefficient AQDC

Fixed-coefficient AQDC adds raw RTT measurements and a **fixed** AQDC compensation coefficient. The coefficient does not adapt yet. Read the [simple implementation walkthrough](doc/aqdc-fixed-validation.md).

**Presentation outputs:**
- `analysis/figures/aqdc-fixed/aqdc-fixed-cwnd.png`
- `analysis/figures/aqdc-fixed/aqdc-fixed-rtt.png` (raw RTT)
- `analysis/figures/aqdc-fixed/aqdc-fixed-queue.png`
- `analysis/figures/aqdc-fixed/aqdc-fixed-throughput.png` (receiver goodput)
- `analysis/aqdc-fixed/aqdc-fixed-metrics.csv` (one main numerical summary)

There are exactly four Fixed-coefficient AQDC graphs in one folder, each showing only original AQDC (alpha=1) versus fixed AQDC (alpha=1.5). The 1.5 value illustrates stronger compensation; it is not claimed optimal. The metrics CSV retains the full six-controller sweep. Publishing another comparison replaces this set instead of creating graph folders for each run. `analysis/aqdc-fixed/aqdc-fixed-current-run.json` identifies the exact source manifest. Versioned manifests, console logs and episode CSVs remain supporting evidence; they are not additional presentation outputs. Old Week 1/2 plots remain separate.

Recreate the existing four presentation graphs without rerunning simulations:

```bash
.venv/bin/python analysis/aqdc_fixed_analysis.py --analyse analysis/aqdc-fixed/aqdc-fixed-duration-120s-v2/aqdc-fixed-run-manifest.json --publish
```

Run a fresh complete comparison:

```bash
./ns3 build test-runner bbr-baseline-experiment aqdc-baseline-experiment aqdc-fixed-experiment -j 4
.venv/bin/python analysis/aqdc_fixed_analysis.py --duration 120
```

Each simulation run still contains exactly four `.dat` files. Fixed-coefficient AQDC `rtt.dat` has the versioned columns `time_s raw_estimator_rtt_ms smoothed_rtt_ms`. Reanalysis without `--publish` updates numerical supporting evidence only, so reviewing the longer run does not replace the presentation graphs.
