# Week 2: reconstructing original AQDC-BBRv1

## Purpose and attribution

Week 1 established an unmodified BBRv1 measurement baseline. Week 2 implements and validates the original AQDC mechanism, without the adaptive coefficient planned for Week 3. This is our reconstruction from the paper, not the authors' source code or a claim of a new algorithm.

Reference: Ziyi Li et al., *A Queue-Aware Extension to BBR for Low-Latency Transport in Video Applications*, IET Communications, published 30 March 2026, [DOI 10.1049/cmu2.70141](https://doi.org/10.1049/cmu2.70141). The implementation follows Algorithm 1 and Sections 3.3--3.4; it does not attempt to reproduce every evaluation scenario or reported percentage.

## 1. Debt arithmetic

For an increase in the filtered bandwidth estimate during ProbeBW:

```text
increment_bytes = (new_bandwidth_bps - previous_bandwidth_bps) * min_RTT_seconds / 8
debt_bytes += increment_bytes
```

At ProbeRTT entry:

```text
BDP_bytes = current_bandwidth_bps * min_RTT_seconds / 8
target_bytes = max(4 * MSS_bytes, floor(BDP_bytes - debt_bytes))
```

The sender restricts its congestion window until actual bytes in flight are at most this target, then resets debt and restores native BBR operation. There is no adaptive multiplier in Week 2.

## 2. Explicit interpretation choices

The publication is not an executable specification. These choices are important when presenting the reproduction:

1. **Actual inflight versus target.** Equation 7 names the target on the left, making the comparison tautological when combined with Algorithm 1. We compare actual outstanding bytes with the calculated target, matching the surrounding completion description.
2. **Initialization and phase.** The first valid estimate seeds the reference. Startup, Drain and ProbeRTT update that reference without adding debt; Section 3.4 identifies ProbeBW as the accumulation phase. Startup capacity discovery is not charged as probing debt.
3. **Filter input.** We observe native BBR's maximum-filter output. A raw rate sample can rise without changing BBR's model and must not be charged twice.
4. **Entry snapshot.** The target is computed at entry and held until completion, corresponding to Algorithm 1's start operation. Refreshing bandwidth history during repayment does not move that goal.
5. **Bounds.** Four MSS is the stated minimum. An unavailable BDP uses that minimum. Arithmetic subtracts in floating point before clamping, preventing unsigned underflow; an oversized result saturates to the socket's window range.
6. **Where compensation acts.** Some architectural prose describes continuous compensation more broadly. We implement the explicit start/end sequence in Algorithm 1 and Section 3.4, rather than inventing a separate window penalty outside ProbeRTT.

No fixed recovery timeout or extra waiting round is introduced. TCP's native retransmission and loss recovery remain available. A sender with zero outstanding data can complete immediately. Inherited `ProbeRttDuration` is deliberately not consulted by AQDC's handler.

## 3. Reuse native BBR rather than copy it

`TcpAqdcBbr` derives from `TcpBbr`. It preserves bandwidth filtering, pacing gains, ACK aggregation, Startup, Drain, ProbeBW, and the native trigger for ProbeRTT.

Native BBR previously had private model state and nonvirtual internal operations. The integration adds four virtual hooks and narrow protected accessors/utilities; it does not change the bodies of the existing native algorithm. The unchanged baseline is checked using both native tests and trace comparison.

| Hook or method | Responsibility |
| --- | --- |
| `UpdateBottleneckBandwidth` | Run native filtering, then observe the resulting estimate |
| `HandleProbeRTT` | Snapshot target; test actual inflight; complete without the native timer |
| `ModulateCwndForProbeRTT` | Apply the cap at the end of native window updates |
| `ExitProbeRTT` | Clear debt and call the native state transition |
| `CongestionStateSet`, `CwndEvent` | Reapply the cap after native recovery operations that assign or restore a window |

The window cap prevents new transmissions when outstanding data exceeds the target. It does not delete packets already queued at the router. Queue debt is a sender-side estimate, not a measurement of that router queue. The experiment independently samples the real forward bottleneck FIFO.

`CalculateTarget` is a protected virtual method so Week 3 can replace the repayment calculation while reusing this tested lifecycle. Its Week 2 implementation remains strictly one-to-one.

## 4. Walk through an example

Suppose the filtered estimate increases from 4 to 8 Mbps, with a minimum RTT of 100 ms:

- New debt is `(8,000,000 - 4,000,000) * 0.1 / 8 = 50,000` bytes.
- BDP at 8 Mbps is `100,000` bytes.
- The original one-to-one compensation target is `50,000` bytes.
- With `120,000` bytes outstanding, the controller must keep waiting and restrict sending.
- Once acknowledgments reduce outstanding data to `50,000` bytes or less, it completes, resets the debt, and restores the saved window.

If the estimate decreases, that is not proof of queue drainage, so it does not subtract debt. If it later increases again, only the positive increment from the updated reference is added.

## 5. Same experiment, two controllers

Both entry points call `scratch/baseline-experiment.h`, which owns the entire topology and measurement code. Their only simulation differences are the controller TypeId and output-folder prefix:

```bash
./ns3 run bbr-baseline-experiment
./ns3 run aqdc-baseline-experiment
```

Results are `results/bbr-baseline-vN/` and `results/aqdc-baseline-vN/`. Each contains exactly `cwnd.dat`, `rtt.dat`, `queueSize.dat`, and `throughput.dat`. Both use the existing 10 Mbps, 50 ms, 1-BDP, single-flow experiment; these settings differ from parts of the paper's evaluation.

Optional diagnostics print exact controller events to the console, never to extra result files:

```bash
./ns3 run "aqdc-baseline-experiment --diagnostics=1"
.venv/bin/python analysis/validate_aqdc.py
```

`QueueDebt`, `CompensationTarget`, and `Compensation` are available as ns-3 trace sources. The last reports entry/completion with debt, target, and actual inflight bytes. The validator checks that every reported completion satisfies the target, checks paired events, and validates the four output files. These are exact controller events, not polling-based estimates of state transitions.

## 6. Evidence and tests

Build with tests enabled, then run:

```bash
./ns3 configure --enable-tests --disable-examples --enable-modules=applications,flow-monitor,internet,point-to-point,traffic-control
./ns3 build test-runner bbr-baseline-experiment aqdc-baseline-experiment -j 4
./ns3 run "test-runner --suite=tcp-aqdc --verbose"
./ns3 run "test-runner --suite=tcp-bbr-test --verbose"
.venv/bin/python -m unittest discover -s analysis -p 'test_*.py'
.venv/bin/python analysis/compare_baselines.py
```

The AQDC suite exercises seeded initialization, byte conversion, repeated estimates, decreases, non-ProbeBW updates, invalid RTT/rate observations, filtered versus raw rate changes, target floors and saturation, zero-ACK and loss-recovery window paths, zero debt, application-limited drainage, repeated episodes, and independent fork state. It also verifies that completion is possible without waiting for the native 200 ms timer.

The comparison computes interval-weighted receiver goodput and creates four overlays in `analysis/figures/comparison/`. For an exact pair, supply `--bbr-run` and `--aqdc-run`. It rejects shortened runs rather than silently comparing incompatible durations.

## 7. What can be claimed

The deliverable demonstrates an implemented mechanism and explains its effect in one controlled scenario. A lower window or an internal debt reset alone does not prove lower queue delay. Report the measured queue and throughput tradeoff, even if it is unfavorable.

`rtt.dat` deliberately keeps the Week 1 **smoothed RTT change-event** format. Its mean and percentiles are descriptive, not raw per-packet or per-ACK latency statistics. A single deterministic run does not establish statistical significance, fairness, or general superiority. Raw RTT instrumentation and broader evaluation remain Week 3 work; changing the compensation factor is not part of this Week 2 reproduction.

When raw RTT collection is added, rerun every compared controller through the same new measurement path; do not compare a new raw RTT series against these smoothed RTT files.

## 8. Validation recorded on 19 October 2026

Environment: this WSL checkout, ns-3.46.1, default build profile with assertions enabled, seed 1, run 1, fixed settings above.

- Six AQDC test cases pass, covering the groups described above, including a one-second slow-drain check and target-snapshot stability.
- All six existing native `tcp-bbr-test` cases pass.
- Eight Python analysis tests pass.
- All four BBR files in `bbr-baseline-v7` are byte-identical to the pre-extension `bbr-baseline-v5` files. The hooks and shared-harness refactor did not alter that baseline's measured behavior.
- The runtime validator passes on `aqdc-baseline-v1`. It observes 136 positive debt updates and two paired compensation episodes. All existing result folders retain exactly four populated `.dat` files.

The first episode enters at 10.150348255 s with 41,648.21875 bytes of estimated debt, a 61,402-byte target and 97,016 actual inflight bytes. It finishes at 10.181589855 s with 59,368 inflight bytes: a 31.24 ms episode rather than a forced 200 ms wait. The second finishes immediately at 20.238806813 s because 105,704 inflight bytes are already below its 106,672-byte target. No fixed 10-second schedule is added: native RTT-filter aging still controls entry, and subsequent RTT observations can refresh that filter's timestamp.

| Active-window metric | BBR v7 | AQDC v1 |
| --- | ---: | ---: |
| Interval-weighted receiver goodput (Mbps) | 9.342559 | 9.553757 |
| Mean smoothed RTT change-event value (ms) | 85.105553 | 84.659092 |
| Smoothed RTT change-event P99 (ms; not raw RTT) | 98.608116 | 98.101875 |
| Mean sampled forward FIFO occupancy (packets) | 25.652755 | 26.131886 |
| Maximum sampled forward FIFO occupancy (packets) | 41 | 41 |

This result is mixed: goodput and smoothed RTT improve slightly, but mean queue occupancy increases. The internal debt resets at each completed episode; it then accumulates again, leaving 34,345.413 estimated bytes at the end of this run. Neither a debt reset nor this single comparison demonstrates an empty real queue or reproduces the paper's reported improvement percentages.

The comparison script also saves four separate Week 1/Week 2 overlay graphs in `analysis/figures/comparison/`: `cwnd.png`, `rtt.png`, `queue.png`, and `throughput.png`. Blue solid lines show Week 1 BBRv1; orange dashed lines show Week 2 AQDC-BBRv1. Each figure identifies its source runs.
