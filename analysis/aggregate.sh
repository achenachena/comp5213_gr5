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
        if ($i == "timeFirstRxPacket") {
          # Extract numeric value, remove + prefix and ns suffix
          tFirstStr = $(i+1);
          gsub(/^\+/, "", tFirstStr);
          gsub(/ns$/, "", tFirstStr);
          tFirst = tFirstStr + 0;
          # Convert nanoseconds to seconds
          tFirst = tFirst / 1e9;
        }
        if ($i == "timeLastRxPacket") {
          # Extract numeric value, remove + prefix and ns suffix
          tLastStr = $(i+1);
          gsub(/^\+/, "", tLastStr);
          gsub(/ns$/, "", tLastStr);
          tLast = tLastStr + 0;
          # Convert nanoseconds to seconds
          tLast = tLast / 1e9;
        }
      }
      if (flowId != "" && rxBytes != "" && tLast > 0) {
        duration = tLast - warmup;
        if (duration <= 0) {
          duration = tLast - tFirst;
        }
        if (duration > 0 && rxBytes > 0) {
          # Throughput in Mbps: (bytes * 8 bits/byte) / (seconds * 1e6 bits/Mbit)
          throughput = (rxBytes * 8.0) / (duration * 1e6);
          print scenario, tcp, run, flowId, throughput;
        }
      }
    }
  ' "${xml}" >> "${THROUGHPUT_CSV}"

done

# Compute Jain's fairness index per {scenario,tcp,run}
awk -F',' -v fairness_csv="${FAIRNESS_CSV}" 'NR>1 {key=$1"|"$2"|"$3; throughput[key] = throughput[key]" "$5; count[key]++}
END {
  print "scenario,tcp,run,jain_index" > fairness_csv;
  for (key in throughput) {
    split(key, parts, "|");
    split(throughput[key], values, " ");
    sum=0; sumsq=0; n=0;
    for (i in values) {
      if (values[i] == "") continue;
      val = values[i] + 0;
      if (val > 0) {
        sum += val;
        sumsq += val * val;
        n++;
      }
    }
    if (n > 0 && sumsq > 0) {
      jain = (sum * sum) / (n * sumsq);
      print parts[1]","parts[2]","parts[3]","jain >> fairness_csv;
    }
  }
}
' "${THROUGHPUT_CSV}"

echo "[INFO] Aggregated metrics written to ${THROUGHPUT_CSV} and ${FAIRNESS_CSV}" >&2
