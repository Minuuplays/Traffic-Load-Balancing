#!/usr/bin/env bash
# Rebuild everything: compile, run the three experiments, parse the output, draw the figures.
# Run from the repository root:  bash scripts/run_all.sh
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-build}"          # override with BUILD_DIR=/some/dir if needed
mkdir -p "$BUILD_DIR" results/raw
g++ -O2 -std=c++17 src/main.cpp -o "$BUILD_DIR/calb"

# The program keeps ONE random generator (seed 42) for its whole run, so every experiment
# must start in a fresh process to get the same instance each time. Menu choice, then 0 = exit.
printf "1\n0\n" | "$BUILD_DIR/calb" > results/raw/option1_small_example.txt   # small example, 2 steps
printf "3\n0\n" | "$BUILD_DIR/calb" > results/raw/option3_alpha_sweep.txt     # alpha sweep, 200 nodes
printf "4\n0\n" | "$BUILD_DIR/calb" > results/raw/option4_scalability.txt     # 100..5000 nodes

python3 scripts/parse_output.py results
python3 scripts/plot_results.py results results/plots
