# Project Report Outline

## 1. Introduction
- Motivation for comparing TCP congestion control (wired vs. wireless, high-BDP vs. mmWave).
- Summary of survey insights (QtColFair, DLCCA, ns-3 ecosystem) referencing `docs/lit_review.md`.
- Objectives: benchmark classic, modern, and learning-inspired controllers across heterogeneous scenarios.

## 2. Related Work
- Concise synthesis of the four target papers plus any supplementary references used to justify scenario choices.
- Table linking each prior work to the gaps our experiments address (fairness, automation, mmWave validation).

## 3. Methodology
- Description of ns-3 environment (`~/ns3-workspace/ns-3`, compiler flags, version).
- Detailed explanation of `tcp_compare.cc` architecture, scenario parameters (refer to `ns3/experiment_plan.md`).
- Automation tooling (`ns3/tools/run_tcp_matrix.sh`, configuration manifest).
- Data collection and processing pipeline (`analysis/aggregate.sh`, metrics definitions, warm-up handling).
- Module prerequisites (standard ns-3 `lte` module suffices for Scenario S4).

## 4. Results
- Throughput + fairness plots per scenario (import aggregated CSV).
- Latency/cwnd analysis using `cwnd.csv` traces (highlight transient behaviour, blockage recovery once S4 is implemented).
- Discussion on loss resilience (Scenario S3), RTT fairness (Scenario S2), and LTE blockage recovery (Scenario S4).

## 5. Discussion
- Interpretation of trends relative to expectations from literature.
- Limitations: absence of full RL port, pending S4 implementation, queue model simplifications.
- Future work: integrate QtColFair codebase, mmWave module, reinforcement learning controller.

## 6. Conclusion
- Key takeaways and recommendations for deployment contexts (e.g., which TCP variant for long-delay satellites vs. mmWave disaster links).

## Appendices
- Command log excerpts for reproducibility.
- Table mapping config files to git commits.
- Checklist for replicating experiments from clean ns-3 checkout.

---
Use this outline to drive the final report write-up and slide deck. Update sections with figures/tables once simulation results are captured.
