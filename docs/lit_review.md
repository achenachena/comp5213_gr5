# Literature Review: TCP Congestion Control Algorithms

## Paper Summaries

### TCP Congestion Control Algorithm Using Queueing Theory-Based Optimality Equation (Ngwenya et al., 2025)  
- **Source**: Electronics 14(2):263, DOI: 10.3390/electronics14020263.  
- **Core idea**: Derives a closed-form optimality equation for congestion control based on Little's Law, yielding the TCP QtColFair algorithm.  
- **Algorithms evaluated**: QtColFair vs. TCP BBR vs. TCP CUBIC.  
- **Methodology**: Queueing-theoretic derivation backed by packet-level simulations (authors release ns-3-compatible implementation in their GitHub repository). Focuses on steady-state link utilization, queueing delay, and packet loss across large buffer regimes.  
- **Key findings**: QtColFair sustained ~96% bottleneck utilization, outperforming BBR (~94%) and CUBIC (~93%) while matching BBR's low delay performance when buffers are large.  
- **Limitations / relevance**: Needs validation under small buffers, mixed RTTs, and multi-flow fairness scenarios. Paper emphasises theoretical derivation; ns-3 validation parameters (topologies, link capacities) must be gleaned from released code.

### Deep learning-based TCP congestion control algorithm for disaster 5G environment (Kuppusamy et al., 2023)  
- **Source**: Research Square preprint, DOI: 10.21203/rs.3.rs-2446108/v1.  
- **Core idea**: Proposes DLCCA, a deep-learning congestion controller tailored for mmWave 5G disaster-response links. Learns to predict disconnections and adjusts congestion window using RTT-derived queue estimates.  
- **Algorithms evaluated**: DLCCA vs. TCP NewReno, CUBIC, Compound, Westwood.  
- **Methodology**: NS-2 simulations of bidirectional mmWave links subject to random loss and blockage; measures throughput resilience under heavy load.  
- **Key findings**: DLCCA maintains higher throughput than legacy algorithms across tested random loss rates by avoiding unnecessary window collapse.  
- **Limitations / relevance**: Uses NS-2 rather than ns-3; lacks fairness, delay, and coexistence metrics. Provides insight into RL-inspired controllers, but porting to ns-3 requires reimplementation of learning agent and channel model.

### Implementation and validation of TCP options and congestion control algorithms for ns-3 (Casoni et al., 2016)  
- **Source**: WNS3 2015 Proceedings, DOI: 10.1145/2756509.2756518.  
- **Core idea**: Extends ns-3's TCP stack with missing options (window scaling, timestamps) and adds high-BDP congestion control variants (CUBIC, Hybla, HighSpeed, BIC, Noordwijk) to align simulator behaviour with modern stacks.  
- **Methodology**: Implements algorithms within ns-3, validates them against Linux traces and analytical expectations across Ethernet and satellite-like links.  
- **Key findings**: Demonstrates close match between ns-3 models and reference implementations, enabling accurate simulation of high-delay/high-bandwidth scenarios.  
- **Limitations / relevance**: Focuses on correctness validation; does not provide comparative performance study or automation. Serves as the foundational code base we will rely on when selecting congestion control variants in our ns-3 experiments.

### TCP Evaluation Suite for ns-3 (Mishra et al., 2016)  
- **Source**: WNS3 2016 Proceedings, DOI: 10.1145/2915371.2915388.  
- **Core idea**: Ports the ICCRG "Common TCP Evaluation Suite" to ns-3, automating scenario creation, traffic generation, execution, and metrics collection for congestion-control benchmarking.  
- **Methodology**: Provides scripts to generate canonical scenarios: single/multi bottleneck topologies, varying bottleneck bandwidth, RTT, and number of long flows. Demonstrates usage on five TCP variants already in ns-3.  
- **Key findings**: Reduces manual effort to run consistent TCP experiments and ensures reproducible metrics (throughput, loss, fairness proxies).  
- **Limitations / relevance**: Targets ns-3 APIs circa 2016; utility scripts may need updates for the current release, and it does not include newer algorithms (BBR, QtColFair, DL-based controllers).

## Cross-Paper Comparison

| Paper | Algorithms Covered | Simulation Platform | Primary Metrics | Scenario Highlights | Identified Gaps | Notes for Our Work |
| --- | --- | --- | --- | --- | --- | --- |
| Ngwenya et al. (2025) | QtColFair, BBR, CUBIC | Analytical + custom simulations (code indicates ns-3 support) | Throughput, queueing delay, packet loss | Single bottleneck with large buffers; focus on steady state | No fairness or mixed RTT analysis | Reproduce with ns-3 to verify fairness/latency trade-offs under varied buffers |
| Kuppusamy et al. (2023) | DLCCA, NewReno, CUBIC, Compound, Westwood | NS-2 | Throughput under random loss | mmWave 5G disaster links with blockage | No delay/fairness metrics; not ns-3 | Adapt concept by prototyping a learning-based controller in ns-3 or comparing against RL baselines like Copa/C2TCP |
| Casoni et al. (2016) | CUBIC, Hybla, HighSpeed, BIC, Noordwijk (plus TCP options) | ns-3 core | Validation accuracy | Ethernet and satellite-emulated links | No comparative study | Leverage implementations; confirm availability in installed ns-3 build |
| Mishra et al. (2016) | Any ns-3 TCP variant (suite-driven) | ns-3 + automation scripts | Throughput, loss, fairness proxies | ICCRG benchmark scenarios (single/multi bottlenecks, variable RTT/bandwidth) | Scripts may require modernization | Use suite as baseline harness; extend to include QtColFair/BBR and 5G-like topologies |

## Takeaways for Experiment Design

- **Algorithm shortlist**: Start with ns-3 built-ins (NewReno, CUBIC, BBR if available, Hybla/HighSpeed) and add QtColFair or RL-inspired variants if feasible.  
- **Scenario coverage**: Combine ICCRG suite topologies with disaster-style mmWave links to contrast wired vs. wireless extremes.  
- **Metrics**: Beyond throughput, capture RTT distribution, queue occupancy, fairness (Jain index), and responsiveness to loss vs. delay signals.  
- **Tooling**: Reuse or modernize the TCP Evaluation Suite scripts to automate batch runs; ensure compatibility with current ns-3 version (`~/ns-3`).  
- **Research gap**: None of the papers simultaneously examine fairness, latency, and adaptability across heterogeneous RTTs in ns-3—goal for our study.
