# Stage 1 — simple implementation and presentation walkthrough

## Open these outputs

There is one presentation set, with exactly four graphs:

```
analysis/figures/stage1/
    stage1-cwnd.png
    stage1-rtt.png
    stage1-queue.png
    stage1-throughput.png

analysis/stage1/stage1-metrics.csv
```

All four graphs show exactly two curves on the same 120 s experiment: original AQDC (alpha=1, blue solid) and Stage 1 fixed AQDC (alpha=1.5, orange dashed). The illustrative 1.5 setting demonstrates stronger compensation; it is not a claimed optimum. The complete six-controller sweep, including the original/fixed-1 equivalence check, remains in the metrics CSV and supporting evidence. RTT means raw accepted RTT-estimator samples; throughput means useful application goodput received. The main CSV is the numerical summary of those same runs. `stage1-current-run.json` identifies their source manifest.

The extra CDF, metric bar charts, compensation charts, zoom plots and repeated per-run graph folders have been removed. The generator cannot create them now. Publishing a new comparison updates these same four filenames. Historical Week 1/2 graphs are separate.

## What Stage 1 does

Stage 1 asks: **what changes if AQDC repays the estimated debt more or less strongly, using a constant coefficient?** Stage 2, which is not implemented, will ask whether that coefficient should change automatically using delay observations.

### Step 1 — measure RTT correctly

Old Week 1/2 RTT data contained changes in the smoothed RTT estimate. That is not the same as collecting all valid raw measurements. Added a `RttSample` callback to TCP immediately after a valid RTT estimator input. It records the input and resulting smoothed RTT, including consecutive equal samples.

Stage 1 `rtt.dat` is:

```
# stage1-rtt-v1 time_s raw_estimator_rtt_ms smoothed_rtt_ms
# controller=... coefficient=... stop_s=120 start_s=0.1 seed=1 run=1
0.15 50 50
```

This is one observation per accepted estimator input, not per transmitted packet or every ACK. Native sample selection, timestamp quantisation and retransmission ambiguity handling are unchanged. Valid handshake observations within the time window are included. Old runs are kept in their original two-column format and are not mixed with raw samples.

Files edited: `src/internet/model/tcp-socket-base.h`, `.cc` and `scratch/baseline-experiment.h`.

### Step 2 — add a fixed compensation coefficient

New class `TcpAqdcFixed` inherits the original AQDC debt accounting and compensation lifecycle. It changes only target calculation:

```
target = max(4*MSS, floor(estimated_BDP - coefficient*estimated_debt))
```

The coefficient is a positive finite parameter and stays constant for the whole run. There is no beta, delay-based update, or adaptive coefficient in Stage 1. A target is held fixed for each episode. A new/forked flow retains the configured coefficient but has independent debt history.

Files added: `src/internet/model/tcp-aqdc-fixed.h` and `.cc`.
Files edited: `src/internet/model/tcp-aqdc-bbr.h` and `.cc`, which share safe arithmetic; original AQDC always passes coefficient 1. `src/internet/CMakeLists.txt` registers the new files.

### Step 3 — use the same experiment for every controller

The new `scratch/aqdc-fixed-experiment.cc` is a small entry point into the existing shared harness. Controller choice and coefficient change; the network stays matched: one TCP flow, 100 Mbps access links, 10 Mbps bottleneck, 50 ms propagation RTT, 62500-byte FIFO, MSS 1448, seed/run 1, application 0.1--120 s and one second of receiver drain.

Example:

```bash
./ns3 run "aqdc-fixed-experiment --coefficient=1.5 --diagnostics=1"
```

Results use `results/stage1-...-vN/`, with exactly `cwnd.dat`, `rtt.dat`, `queueSize.dat`, `throughput.dat`. Duration is configurable; default is 120 s. Historical commands still produce their old formats/prefixes.

Files: new wrapper, edited `scratch/baseline-experiment.h`, `scratch/.gitignore` and root `.gitignore`.

### Step 4 — run the comparison and calculate metrics

`analysis/stage1_fixed_coefficients.py` runs the six settings, validates episode completion and checks that fixed coefficient 1 reproduces original AQDC across all four measured traces. It publishes only four two-curve overlays and one main CSV containing all six tested settings. BBRv1 and other coefficients remain in that CSV rather than crowding the graphs.

The CSV includes raw RTT mean/median/P95/P99, interval-weighted goodput, time-weighted sampled queue occupancy, sampled queue maximum, and compensation counts/durations. All metrics use [0.1, application stop). RTT percentiles weight each accepted sample equally, using NumPy linear interpolation. Queue means use a piecewise-constant reconstruction of 0.2 s samples; peaks between samples can be missed. Goodput includes the final partial sampling interval.

Diagnostics verify actual inflight <= target at completion, paired entry/exit events, fixed episode targets and agreement with controller summary counts. Unfinished episodes are reported explicitly. Supporting logs/manifests/episode CSVs remain in versioned `analysis/stage1/stage1-duration-.../` folders for reproducibility. You do not need these folders for the presentation.

### Step 5 — validate correctness

- 7 AQDC cases, including fixed-coefficient arithmetic, minimum target, fork configuration, target snapshot and completion.
- 6 native BBR cases.
- 10 native RTT-estimation integration cases, checking callback counts against actual estimator sample counts with/without timestamps and with retransmissions.
- 16 Python analysis tests.
- Fixed coefficient 1 exactly matches original AQDC; original headers identify different controller names, but numeric RTT values match and the other three files are byte-identical.
- Both historical BBR and original AQDC outputs remain byte-identical to their references.
- The repeated 120 s comparison produces identical files, confirming reproducibility rather than independent statistical evidence.

Tests edited: `src/internet/test/tcp-aqdc-test.cc`, `src/internet/test/tcp-rtt-estimation.cc`.
Test file added: `analysis/test_stage1_fixed_coefficients.py`.
Validation record: `analysis/stage1/stage1-validation.txt`.

## What the experiment actually showed

The first episode starts at 10.150348255 s with debt 41648.21875 bytes and inflight 97016 bytes. Its evolving estimated BDP is approximately 103050.75 bytes; this is not the configured propagation BDP of 62500 bytes.

| Constant coefficient | Target (bytes) | First compensation duration (ms) |
| ---: | ---: | ---: |
| 0.5 | 82226 | 14.4192 |
| 1 / original AQDC | 61402 | 31.2416 |
| 1.5 | 40578 | 48.0640 |
| 2 | 19754 | 64.8864 |

A larger coefficient lowers the target and extends drainage. This demonstrates the mechanism, not a guaranteed performance improvement.

| Controller | Goodput (Mbps), 120 s | Raw RTT-sample P99 (ms) |
| --- | ---: | ---: |
| BBRv1 | 9.342559 | 101 |
| Original AQDC | 9.553757 | 101 |
| Fixed 0.5 | 9.553853 | 101 |
| Fixed 1 | 9.553757 | 101 |
| Fixed 1.5 | 9.552404 | 101 |
| Fixed 2 | 9.551051 | 101 |

Only two compensation episodes occur per AQDC variant, even in the supporting 600 s comparison. P99 remains 101 ms in that longer experiment. There is no demonstrated P99 improvement or optimal coefficient. Stage 2 must determine whether adaptation adds value beyond these fixed references. No adaptive performance result has been created or claimed.

## Commands

Build and run a new comparison; it replaces the four presentation graphs and main CSV:

```bash
./ns3 build test-runner bbr-baseline-experiment aqdc-baseline-experiment aqdc-fixed-experiment -j 4
.venv/bin/python analysis/stage1_fixed_coefficients.py --duration 120
```

Regenerate the current presentation without simulations:

```bash
.venv/bin/python analysis/stage1_fixed_coefficients.py --analyse analysis/stage1/stage1-duration-120s-v2/stage1-run-manifest.json --publish
```

Omit `--publish` when reviewing another manifest to avoid replacing the presentation set. Exactly which result folders produced it is recorded in `stage1-current-run.json` and the referenced manifest. No changes were committed or pushed.

## Suggested professor explanation

"Stage 1 establishes raw RTT measurement and introduces a constant debt multiplier. We first verify that coefficient 1 reproduces original AQDC, then compare weaker and stronger repayment under the same network settings. Larger coefficients lower the compensation target and extend drainage, but P99 does not improve in this fixed setup. These tested references are the basis for Stage 2, where we will investigate changing the coefficient using delay observations."
