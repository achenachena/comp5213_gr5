# Simulation Output Post-Processing

## Directory Layout
- `aggregate.sh`: Bash script that scans the ns-3 `results/` tree, extracts FlowMonitor statistics, and computes per-flow throughput and Jain fairness indices.
- `out/`: Created by the aggregator; stores CSV tables ready for plotting.

## Usage
1. Run the simulation sweep (e.g. via `ns3/tools/run_tcp_matrix.sh`).
2. Export `RESULT_ROOT` if ns-3 results are stored outside the default path (`~/ns-3/results`).
3. Execute the aggregator:
   ```bash
   cd analysis
   ./aggregate.sh
   ```
   Optional environment variables:
   - `RESULT_ROOT=/absolute/path/to/results`
   - `OUTPUT_ROOT=/absolute/path/to/output`
   - `WARMUP=20` (seconds to discard per run)
4. Inspect generated CSV files in `out/` and feed them into `gnuplot` or spreadsheet tooling. Example `gnuplot` snippet:
   ```gnuplot
   set datafile separator ','
   set terminal png size 1280,720
   set output 'throughput_s1.png'
   plot 'out/throughput.csv' u (strcol(1) eq 'S1' && strcol(2) eq 'TcpCubic' ? $5 : 1/0) w boxes title 'CUBIC', \
        'out/throughput.csv' u (strcol(1) eq 'S1' && strcol(2) eq 'TcpNewReno' ? $5 : 1/0) w boxes title 'NewReno'
   ```

## Notes
- FlowMonitor XML parsing relies on standard ns-3 attribute ordering; if you extend the program with additional metrics ensure `aggregate.sh` still locates `<Flow>` elements correctly.
- Scenario `S4` uses the built-in LTE helper to emulate blockage; pass `BLOCKAGE` to `run_tcp_matrix.sh` (defaults to `0.2` seconds) to sweep alternative outage lengths.
