# Slide Deck Blueprint

1. **Title & Team**
   - Project name, course, contributors, ns-3 version.

2. **Problem Statement**
   - Visual showing congestion control need; summarize research gap uncovered in literature.

3. **Algorithm Portfolio**
   - Quick table comparing NewReno, CUBIC, Hybla, HighSpeed, BBR, QtColFair, DLCCA proxy.

4. **Experimental Design**
   - Diagram of S1–S5 topologies; highlight parameters (bandwidth, delay, loss).

5. **Automation Workflow**
   - Flowchart from `run_tcp_matrix.sh` → ns-3 results → `aggregate.sh` → plots.

6. **Key Results (per scenario)**
   - Throughput & fairness charts; annotate notable behaviours (e.g., BBR fairness issues, QtColFair utilization gains).

7. **Wireless Blockage Findings**
   - Summarize Scenario S4 LTE blockage behaviour and recovery time.

8. **Takeaways**
   - Bullet list of recommendations for choosing TCP variants under different conditions.

9. **Future Work & Risks**
   - Items left to implement (QtColFair integration, full DLCCA port, queue disciplines).

10. **Q&A**
    - Backup slides with configuration tables, reproducibility notes.
