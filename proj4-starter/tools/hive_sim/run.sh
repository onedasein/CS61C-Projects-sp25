#!/usr/bin/env bash
# Local stand-in for:  make task_1|task_2 TEST=tests/<name>
# (builds the requested binary, regenerates inputs + ref.bin with the
#  simulated-hive oracle, runs the binary, and checks out.bin against ref.bin)
set -eu
cd "$(dirname "$0")/../.."

TASK="${1:?usage: run.sh <task_1|task_2> <test_name_or_path>}"
TEST="${2:?usage: run.sh <task_1|task_2> <test_name_or_path>}"

case "$TASK" in
  task_1) COORD=naive;  COMP=naive ;;
  task_2) COORD=naive;  COMP=optimized ;;
  *) echo "unknown task '$TASK' (use task_1 or task_2)"; exit 1 ;;
esac

TEST_NAME="$(basename "$TEST")"
TEST_PATH="tests/$TEST_NAME"
BIN="convolve_${COORD}_${COMP}"

# Always go through make: it rebuilds only when a source is newer and runs
# the starter-file hash gate (we never edit starter files, so it stays green).
make "$BIN" COORDINATOR="$COORD" COMPUTE="$COMP"

# Regenerate inputs + references. Tasks that already have a ref.bin skip the
# oracle (deterministic regeneration, like on hive). Tasks without a ref.bin
# (e.g. a stale test_example from a crashed oracle-less run) are forced to
# regenerate it by dropping their stale .hashes.json first.
if [ -d "$TEST_PATH" ]; then
  for d in "$TEST_PATH"/*/; do
    [ -f "$d/ref.bin" ] || rm -f "$d/.hashes.json"
  done
fi
python3 tools/hive_sim/sim.py "$TEST_NAME"

rm -f "$TEST_PATH"/*/out.bin
bash tools/run_test.sh "./$BIN" "$TEST_PATH/input.txt"
bash tools/check_output.sh "$TEST_PATH"

if [ -f "$TEST_PATH/gifs.json" ]; then
  bash tools/run_python.sh tools/results_to_gif.py "$TEST_PATH"
fi
