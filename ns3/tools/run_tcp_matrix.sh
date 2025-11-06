#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)
NS3_ROOT=${NS3_ROOT:-$HOME/ns3-workspace/ns-3}
SCRATCH_PATH="${NS3_ROOT}/scratch"
PROGRAM_NAME=${PROGRAM_NAME:-tcp_compare}
SCENARIOS=${SCENARIOS:-"S1 S2 S3 S4 S5"}
TCP_VARIANTS=${TCP_VARIANTS:-"TcpNewReno TcpCubic TcpHybla TcpHighSpeed"}
RUNS=${RUNS:-3}
QUEUE_SIZE=${QUEUE_SIZE:-150p}
LOSS_SET=${LOSS_SET:-"0.0 0.01 0.05"}
BLOCKAGE_SET=${BLOCKAGE_SET:-"0.05 0.2 0.5"}
FLOW_MONITOR=${FLOW_MONITOR:-true}

if [[ ! -d "${NS3_ROOT}" ]]; then
  echo "[ERROR] ns-3 root directory not found: ${NS3_ROOT}" >&2
  exit 1
fi

mkdir -p "${SCRATCH_PATH}"
cp "${PROJECT_ROOT}/tcp_compare.cc" "${SCRATCH_PATH}/${PROGRAM_NAME}.cc"

pushd "${NS3_ROOT}" >/dev/null

./ns3 configure --disable-tests >/dev/null
./ns3 build >/dev/null

for scenario in ${SCENARIOS}; do
  for tcp in ${TCP_VARIANTS}; do
    case "${scenario}" in
      S3)
        loss_values=${LOSS_SET}
        blockage_values="0.0"
        ;;
      S4)
        loss_values="0.0"
        blockage_values=${BLOCKAGE_SET}
        ;;
      *)
        loss_values="0.0"
        blockage_values="0.0"
        ;;
    esac
    for loss in ${loss_values}; do
      for blockage in ${blockage_values}; do
        for run in $(seq 1 ${RUNS}); do
          echo "[INFO] Running scenario=${scenario} tcp=${tcp} loss=${loss} blockage=${blockage} run=${run}" >&2
          ./ns3 run "scratch/${PROGRAM_NAME} --scenario=${scenario} --tcp=${tcp} --queue=${QUEUE_SIZE} --run=${run} --loss=${loss} --blockage=${blockage} --flowMonitor=${FLOW_MONITOR}" >/dev/null
        done
      done
    done
  done
done

popd >/dev/null

echo "[INFO] Simulation matrix complete. Results stored under ${NS3_ROOT}/results" >&2
