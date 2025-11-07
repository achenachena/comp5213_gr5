# Comparison of TCP Congestion Control Algorithms

Course project for **COMP-5213-FDE – Computer Networks (Fall 2025), Lakehead University**. We develop and evaluate multi-scenario ns-3 simulations to study how different TCP congestion control algorithms behave across diverse network conditions.

---

## Repository Structure

```
docs/
  lit_review.md         ── summaries and comparisons of the four reference papers
  report/               ── outline for the final report and slide deck
ns3/
  tcp_compare.cc        ── reusable ns-3 scratch program covering multiple scenarios (S1–S5)
  experiment_plan.md    ── detailed design notes (algorithms, scenarios, metrics)
  experiment_matrix.yaml│
  tools/run_tcp_matrix.sh┘ automation script for batch simulations
analysis/
  aggregate.sh          ── FlowMonitor post-processing (throughput, fairness CSV)
  README.md             ── usage guide for the analysis script
```

---

## Highlighted Papers

The project is guided by four primary papers (summaries in `docs/lit_review.md`):

1. **Queueing-Theory Optimal TCP (QtColFair)** — Ngwenya et al., Electronics 2025  
2. **DLCCA Deep-Learning TCP** — Kuppusamy et al., Research Square 2023  
3. **ns-3 TCP Extensions** — Casoni et al., WNS3 2015  
4. **TCP Evaluation Suite for ns-3** — Mishra et al., WNS3 2016

These references motivate our algorithm selection, simulation topologies, and performance metrics.

---

## Scenarios Implemented in `tcp_compare.cc`

| ID | Purpose | Summary |
|----|---------|---------|
| **S1** | ICCRG single bottleneck | Dumbbell topology, long-lived bulk flows, varying queue sizes |
| **S2** | RTT fairness | High/low RTT senders plus short web traffic |
| **S3** | Random loss / wireless proxy | Gilbert-Elliott loss model, UDP cross traffic |
| **S4** | LTE blockage scenario | EPC + LTE helper, 50 Mbps downlink video throttled during blockage |
| **S5** | Multi-flow scalability | Eight senders/receivers with mixed workloads |

Each scenario records congestion window traces and FlowMonitor statistics for post-analysis.

---

## Prerequisites

- macOS or Linux with a working ns-3 installation (tested with ns-3.40+).  
- GCC/Clang toolchain, CMake, and Python (ns-3 default requirements).  
- Scenario S4 relies only on the stock `lte` module—no extra downloads needed.

Ensure your ns-3 tree (e.g., `~/ns-3`) builds successfully before running the experiments.

---

## Quick Start

1. **Copy the scratch program**

   ```bash
   cp ns3/tcp_compare.cc ~/ns-3/scratch/
   ```

2. **Configure & build ns-3**

   ```bash
   cd ~/ns-3
   ./ns3 configure          # add flags like --enable-examples as required
   ./ns3 build
   ```

3. **Run a single scenario**

   ```bash
   ./ns3 run "scratch/tcp_compare --scenario=S4 --tcp=TcpCubic --blockage=0.2"
   ```

   Results are written to `~/ns-3/results/<scenario>/<tcp>/run-<n>/`.

4. **Batch sweep**

   ```bash
   cd /path/to/comp5213_gr5
   ns3/tools/run_tcp_matrix.sh
   ```

   Override defaults as needed:

   ```bash
   SCENARIOS="S1 S4" TCP_VARIANTS="TcpNewReno TcpCubic" RUNS=5 BLOCKAGE=0.5 ns3/tools/run_tcp_matrix.sh
   ```

---

## Post-Processing

1. Run the aggregator:

   ```bash
   cd analysis
   ./aggregate.sh        # set RESULT_ROOT if results live elsewhere
   ```

2. Inspect `analysis/out/throughput.csv` and `analysis/out/fairness.csv` for per-flow throughput and Jain’s fairness index.

3. Plot results using your preferred tool (see `analysis/README.md` for a Gnuplot example).

---

## Documentation Assets

- **`docs/lit_review.md`** — detailed summaries, comparison tables, and experimental takeaways.  
- **`docs/report/outline.md`** — structure for the written report (Introduction, Methodology, Results, etc.).  
- **`docs/report/slides_outline.md`** — blueprint for the presentation deck.

---

## Future Work

- Integrate QtColFair and BBR implementations into ns-3 for broader comparisons.  
- Enhance Scenario S4 with richer blockage traces or additional traffic mixes.  
- Finalise plots and write the report/presentation using the outlines provided.

---

For questions or collaboration details, contact COMP5213 Group 5.***