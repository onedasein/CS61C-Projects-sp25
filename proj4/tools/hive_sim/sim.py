#!/usr/bin/env python3
"""
Hive-machine simulator for CS61C proj4's test generation.

tools/framework.py expects the staff oracle binary at
/home/ff/cs61c/fa24/proj4/convolve_oracle (hive-only).  This launcher
monkey-patches framework.oracle_path to the local stand-in oracle binary
(hive_sim/convolve_oracle) and then runs the unmodified tools/create_tests.py,
so inputs AND reference outputs (ref.bin) are generated exactly like on hive.

Usage:  python3 tools/hive_sim/sim.py [same args as create_tests.py]
        e.g.  python3 tools/hive_sim/sim.py test_tiny
"""
import os
from pathlib import Path
import runpy
import sys

HIVE_SIM_DIR = os.path.dirname(os.path.abspath(__file__))
TOOLS_DIR = os.path.dirname(HIVE_SIM_DIR)
ORACLE = os.path.join(HIVE_SIM_DIR, "convolve_oracle")


def main():
    if not os.path.exists(ORACLE):
        sys.stderr.write(
            "Local oracle not found at %s\n"
            "Build it first:  bash tools/hive_sim/build_oracle.sh\n" % ORACLE)
        return 1

    sys.path.insert(0, TOOLS_DIR)  # so `from framework import ...` works
    import framework
    # Simulate the hive-only staff binary path. framework.run_oracle() reads
    # this module global at call time, so patching before create_tests runs is
    # enough -- tools/create_tests.py stays byte-for-byte untouched.
    framework.oracle_path = Path(ORACLE)  # framework expects a pathlib.Path

    create_tests = os.path.join(TOOLS_DIR, "create_tests.py")
    sys.argv = [create_tests] + sys.argv[1:]
    runpy.run_path(create_tests, run_name="__main__")
    return 0


if __name__ == "__main__":
    sys.exit(main())
