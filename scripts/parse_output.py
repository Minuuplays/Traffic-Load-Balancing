#!/usr/bin/env python3
"""Turn the raw program output in results/raw/ into CSV files.

Usage: python3 scripts/parse_output.py [results_dir]      (default: results)

Reads   results/raw/option1_small_example.txt, option3_alpha_sweep.txt, option4_scalability.txt
Writes  results/small_example.csv, results/alpha_sweep.csv, results/scalability.csv
The program calls the proposed method "Proposed"; the CSV files call it "CALB" (the name used in the report).
"""
import csv, os, re, sys

RES = sys.argv[1] if len(sys.argv) > 1 else "results"
RAW = os.path.join(RES, "raw")
NAME = {"Shortest Path": "Shortest Path", "Greedy LB": "Greedy LB", "Proposed": "CALB"}
HEAD = re.compile(r"=+ COMPARISON \((\d+) nodes, (\d+) roads, (\d+) time steps")
ROW = re.compile(r"^(Shortest Path|Greedy LB|Proposed)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+(\d+)\s+(\d+)\s+(\d+)\s+([\d.]+)\s*$")
AROW = re.compile(r"^alpha=(\S+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+(\d+)\s+(\d+)\s+(\d+)\s+([\d.]+)\s*$")
COLS = ["algorithm", "max_load", "avg_load", "variance", "total_distance", "overload", "unrouted", "time_ms"]

def comparison_rows(path):
    out, cur = [], None
    for line in open(path):
        h = HEAD.search(line)
        if h:
            cur = tuple(int(x) for x in h.groups())          # nodes, roads, steps
            continue
        r = ROW.match(line)
        if r and cur:
            out.append(list(cur) + [NAME[r.group(1)]] + [float(x) if "." in x else int(x) for x in r.groups()[1:]])
    return out

def write(name, header, rows):
    with open(os.path.join(RES, name), "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n"); w.writerow(header); w.writerows(rows)
    print(f"wrote {name} ({len(rows)} rows)")

write("scalability.csv", ["nodes", "roads", "steps"] + COLS, comparison_rows(os.path.join(RAW, "option4_scalability.txt")))
write("small_example.csv", ["nodes", "roads", "steps"] + COLS, comparison_rows(os.path.join(RAW, "option1_small_example.txt")))
alpha = []
for line in open(os.path.join(RAW, "option3_alpha_sweep.txt")):
    m = AROW.match(line)
    if m: alpha.append([float(m.group(1))] + [float(x) if "." in x else int(x) for x in m.groups()[1:]])
write("alpha_sweep.csv", ["alpha"] + COLS[1:], alpha)
