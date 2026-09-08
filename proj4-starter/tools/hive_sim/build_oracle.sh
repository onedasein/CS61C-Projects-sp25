#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")"
gcc -O2 -std=c99 -Wall -Wextra convolve_oracle.c -o convolve_oracle
echo "Built $(pwd)/convolve_oracle"
