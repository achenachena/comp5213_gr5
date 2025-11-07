#!/usr/bin/env bash
set -euo pipefail

RESULT_ROOT=${RESULT_ROOT:-$HOME/ns-3/results}
OUTPUT_ROOT=${OUTPUT_ROOT:-$(pwd)/out}
WARMUP=${WARMUP:-20}

if [[ ! -d "${RESULT_ROOT}" ]]; then
  echo "[ERROR] Result directory not found: ${RESULT_ROOT}" >&2
  exit 1
fi

mkdir -p "${OUTPUT_ROOT}"
THROUGHPUT_CSV="${OUTPUT_ROOT}/throughput.csv"
FAIRNESS_CSV="${OUTPUT_ROOT}/fairness.csv"

echo "scenario,tcp,run,flowId,throughput_mbps" > "${THROUGHPUT_CSV}"

find "${RESULT_ROOT}" -name flowmon.xml | while read -r xml; do
  run_dir=$(dirname "${xml}")
  tcp_dir=$(dirname "${run_dir}")
  scenario_dir=$(dirname "${tcp_dir}")

  run=$(basename "${run_dir}" | sed 's/run-//')
  tcp=$(basename "${tcp_dir}")
  scenario=$(basename "${scenario_dir}")

  awk -v warmup="${WARMUP}" -v scenario="${scenario}" -v tcp="${tcp}" -v run="${run}" '
    BEGIN { FS="[=\" ]+"; OFS=","; }
    /<Flow / {
      flowId=""; rxBytes=""; tFirst=""; tLast="";
      for (i = 1; i <= NF; ++i) {
        if ($i == "flowId") flowId = $(i+1);
        if ($i == "rxBytes") rxBytes = $(i+1);
        if ($i == "timeFirstRxPacket") tFirst = $(i+1);
        if ($i == "timeLastRxPacket") tLast = $(i+1);
      }
      if (flowId != "" && rxBytes != "" && tLast != "") {
        duration = tLast - warmup;
        if (duration <= 0) {
          duration = tLast - tFirst;
        }
        if (duration > 0) {
          throughput = (rxBytes * 8) / (duration * 1e6);
          print scenario, tcp, run, flowId, throughput;
        }
      }
    }
  ' "${xml}" >> "${THROUGHPUT_CSV}"

done

# Compute Jain's fairness index per {scenario,tcp,run}
awk -F',' 'NR>1 {key=$1"|"$2"|"$3; throughput[key] = throughput[key]" "$5; count[key]++}
END {
  print "scenario,tcp,run,jain_index" > "${FAIRNESS_CSV}";
  for (key in throughput) {
    split(key, parts, "|");
    split(throughput[key], values, " ");
    sum=0; sumsq=0; n=0;
    for (i in values) {
      if (values[i] == "") continue;
      val = values[i] + 0;
      sum += val;
      sumsq += val * val;
      n++;
    }
    if (n > 0 && sumsq > 0) {
      jain = (sum * sum) / (n * sumsq);
      print parts[1]","parts[2]","parts[3]","jain >> "${FAIRNESS_CSV}";
    }
  }
}
' "${THROUGHPUT_CSV}"

echo "[INFO] Aggregated metrics written to ${THROUGHPUT_CSV} and ${FAIRNESS_CSV}" >&2
