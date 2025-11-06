# ns-3 Experiment Design: TCP Congestion Control Comparison

Workspace: `~/ns3-workspace/ns-3`

## 1. Algorithms Under Test
- **Baseline**: `TcpNewReno`, `TcpCubic`, `TcpHybla`, `TcpHighSpeed` (all available in stock ns-3 per Casoni et al.).
- **Modern delay-based / hybrid**: `TcpBbr` (verify module availability in installed version; otherwise back-port from ns-3 dev or use third-party patch).
- **Queueing-theoretic variant**: Integrate QtColFair from Ngwenya et al. (2025) using their published ns-3 code if license permits.
- **Learning-inspired placeholder**: replicate the DLCCA idea with a scripted logic (e.g., runtime RTT variance heuristic) if full RL port is infeasible; compare against traditional controllers to highlight gaps.

All runs will expose configuration via command-line arguments so additional algorithms can be appended with minimal code churn.

## 2. Scenario Matrix
| Scenario ID | Purpose | Topology | Link Params | Traffic | Variations |
| --- | --- | --- | --- | --- | --- |
| S1 | ICCRG single bottleneck baseline | Dumbbell (2 senders, 2 receivers, single bottleneck router pair) | Access: 100 Mbps / 1 ms, Bottleneck: 20 Mbps / 30 ms, DropTail queue | Long-lived FTP-style flows (BulkSend) | Bottleneck buffer (BDP/2, 1×BDP, 2×BDP) |
| S2 | RTT fairness stress | Dumbbell with asymmetric propagation delays on left branches | Same as S1 but sender0 RTT 20 ms, sender1 RTT 120 ms | 2 long flows + 2 short web flows per side | Algorithm fairness, queue management |
| S3 | Random loss emulation (wireless) | Point-to-point chain with error model on bottleneck | 40 Mbps / 20 ms, Gilbert-Elliott loss (p=0.02, r=0.5) | Long flow + CBR UDP cross-traffic | Loss rate sweep (0%, 1%, 2%, 5%) |
| S4 | LTE/5G disaster proxy | gNB–UE with EPC (`LteHelper`) | 10 Gbps backhaul, dynamic video throttling to emulate blockage | High-bitrate TCP video (OnOff, 50 Mbps) + uplink bulk flow | Blockage duration sweep (0.05 s, 0.2 s, 0.5 s) |
| S5 | Multi-flow scalability | Dumbbell with 8 senders, 8 receivers | Bottleneck 100 Mbps / 40 ms, RED queue optional | Mixed workloads: 50% FTP, 50% bursty HTTP | Number of active flows (4, 8, 16) |

Each scenario will be repeatable with seed control (`--RngRun`) and run for 120 simulated seconds (first 20 s treated as warm-up).

## 3. Metrics & Instrumentation
- **Throughput**: FlowMonitor per-flow throughput averaged over steady-state (export CSV).
- **Latency**: End-to-end RTT via `Config::Connect("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow", ...)` and timestamped samples; compute average & 95th percentile.
- **Loss rate**: Packet sink counters vs. application send counts.
- **Queue occupancy**: Trace queue length at bottleneck device for Delay/Drop-tail cases.
- **Fairness**: Jain’s fairness index across concurrent TCP flows in each scenario, computed post-simulation.
- **Responsiveness**: For S4 blockage, measure recovery time (seconds to regain 90% of pre-blockage throughput).

All metrics will be emitted via ASCII trace helpers into `results/<scenario>/<algorithm>/` directories.

## 4. Automation Strategy
- Add a driver script (`tools/run_tcp_matrix.sh`) that enumerates `{scenario, algorithm, seed}` tuples and calls `./waf --run "tcp_compare --scenario=S1 --tcp=TcpCubic --run=1"`.
- Post-process traces using a shell + `awk`/`gnuplot` pipeline (no notebook required). Intermediate aggregation stored as CSV via simple helper binaries or shell scripts.
- Maintain a YAML/JSON manifest (`ns3/experiment_matrix.yaml`) listing sweeps so automation can iterate uniformly.

## 5. Validation & Sanity Checks
1. verify each algorithm builds and negotiates features (window scaling, timestamps) by inspecting ns-3 logs.
2. For S1, compare NewReno throughput against Mathis formula to confirm correct BDP behaviour.
3. For scripts imported from QtColFair repo, run author-provided regression tests (if available) before integrating with main driver.

## 6. Timeline & Next Tasks
- **Week 1**: Validate stock ns-3 TCP variants, implement scenario scaffolding (S1–S3). Document configuration defaults in README.
- **Week 2**: Integrate QtColFair (or fall back plan), prototype blockage trace generator for S4. Stabilize automation script.
- **Week 3**: Execute full sweep across scenarios, collect metrics, and draft comparative analysis tables/plots.
- **Throughout**: Maintain experiment logs, commit configs & scripts into `ns3/` with clear version tags.
