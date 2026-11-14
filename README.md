# Traffic Load Balancing with Road Capacity

C++ implementation and evaluation of **CALB (Congestion-Aware Load Balancing)**, which splits vehicle
requests over several paths in a road network under a hard capacity of C = 100 vehicles per road.
CALB is compared with two baselines, **Shortest Path** and **Greedy Load-Balancing**.
The full write-up is in [`report/`](report/).

## Problem

A connected, undirected, weighted graph G = (V, E, w) models the road network. At each time step, requests
(n, s, d) ask to move n vehicles from s to d. Each request is split over one or more s → d paths so that
traffic is balanced, total distance stays small and no road exceeds C.

CALB minimises the convex congestion cost `Cost(e) = w(e) * (L + α·L(L−1)/2)` per road (L = load) using
successive shortest paths (Bellman–Ford) on the residual network, then re-optimises each request against
the others until nothing changes (at most 10 passes).

## Repository layout

| Path | Contents |
|---|---|
| `src/main.cpp` | The C++ program: generator, CALB ("Proposed" in the code), Shortest Path, Greedy LB, metrics, menu |
| `results/raw/` | Program output for menu options 1, 3 and 4 |
| `results/*.csv` | `scalability.csv`, `small_example.csv`, `alpha_sweep.csv`, parsed from the raw output |
| `scripts/run_all.sh` | Compile, run all experiments, parse the output, draw the figures |
| `scripts/parse_output.py`, `scripts/plot_results.py` | Raw output to CSV; CSV to figures (`results/plots/`, not committed) |
| `report/` | The report (DOCX) |

Test instances are not stored: the program generates them from a fixed random seed (42).

## Build and run

```bash
g++ -O2 -std=c++17 src/main.cpp -o calb
./calb            # interactive menu
```

Menu: `1` small example (prints paths and road loads), `2` custom network, `3` alpha sweep (200 nodes),
`4` scalability test (100 to 5,000 nodes).

To rebuild every result and figure in one go (needs g++, Python 3 and `pip install matplotlib`):

```bash
bash scripts/run_all.sh
```

Experiments map to the report as follows: option 4 gives the 5,000-node comparison and the running times,
option 1 the small example (10 nodes, 13 roads, 2 time steps), option 3 the alpha sweep.

## Notes on the implementation

- The "Proposed" method in the program is CALB. Its path search is a queue-based Bellman-Ford (SPFA), which handles the negative backward arcs.
- Greedy LB splits a request into groups of ceil(n/5) vehicles and runs Dijkstra with road cost (load + 1); road length only breaks ties.
- Metrics over the time steps: max load is the worst step, average load and variance are averaged over the steps, total distance, overload count, unrouted vehicles and time are summed.
- Variance is the population variance over all roads (unused roads count as load 0).

## Parameters used in the report

| Parameter | Value |
|---|---|
| Capacity C | 100 |
| Congestion weight α | 0.1 (swept from 0 to 1 on 200 nodes) |
| Time steps | 3 |
| Road lengths | 1–20 |
| Greedy LB groups per request | 5 |
| CALB re-optimisation passes | at most 10 |

## Results (5,000 nodes, 7,498 roads, 150 requests)

| Algorithm | Max load | Avg load | Variance | Total distance | Roads over C | Time (ms) |
|---|---|---|---|---|---|---|
| Shortest Path | 86 | 0.96 | 20.76 | 154,335 | 0 | 58 |
| Greedy LB | 42 | 1.05 | 4.96 | 243,800 (+58%) | 0 | 346 |
| **CALB** | **42** | 1.03 | 6.53 | **177,901 (+15%)** | 0 | 5,043 |

The same values are in `results/scalability.csv` (rows with 5000 nodes). Running times above are the report's.

## Regenerate the figures

```bash
python3 scripts/plot_results.py results results/plots     # writes fig1..fig5 (PNG)
```

The plots are not committed; they are rebuilt from the CSV files.

## Reproducibility notes

- Random seed: 42, fixed in `main()`. The program shares one generator for the whole run, so start a fresh process for each experiment (menu option, then `0`); `run_all.sh` does this.
- Compiler and flags: `g++ -O2 -std=c++17`.
- Metrics (loads, variance, distance) are deterministic. Running times depend on the machine, so they will differ from the report.
- Machine used for the report's timings (CPU, RAM, OS): TODO
- Timing method: wall-clock time of one run, summed over the time steps (`std::chrono::steady_clock`).
- Commit or tag matching the report: TODO

## License

MIT, see [`LICENSE`](LICENSE).
