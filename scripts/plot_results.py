#!/usr/bin/env python3
"""Draw the report figures from the CSV files made by parse_output.py.

Usage: python3 scripts/plot_results.py [results_dir] [output_dir]
       (defaults: results, results/plots)       Needs: pip install matplotlib
"""
import csv, os, sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

RES = sys.argv[1] if len(sys.argv) > 1 else "results"
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(RES, "plots")
MIN_NODES = 500          # the report's running-time chart starts at 500 nodes (the 100-node run is skipped)
CAP = 100
os.makedirs(OUT, exist_ok=True)
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 9, "axes.spines.top": False, "axes.spines.right": False})
ALGS = ["Shortest Path", "Greedy LB", "CALB"]
COL = dict(zip(ALGS, ["#9AA5B1", "#E0A458", "#2F6DB5"]))
thousands = FuncFormatter(lambda v, _: f"{int(v):,}")

def read(name):
    with open(os.path.join(RES, name), newline="") as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        for k, v in r.items():
            try: r[k] = float(v)
            except ValueError: pass
    return rows

def by_alg(rows):
    return {r["algorithm"]: r for r in rows}

def bars(ax, key, rows, title, fmt="{:,.0f}", note=None):
    d = by_alg(rows)
    vals = [d[a][key] for a in ALGS]
    b = ax.bar([a.replace("Shortest ", "Shortest\n") for a in ALGS], vals, color=[COL[a] for a in ALGS], width=0.6)
    for r, v, a in zip(b, vals, ALGS):
        txt = fmt.format(v) + (f"\n({note(a)})" if note and note(a) else "")
        ax.text(r.get_x() + r.get_width() / 2, v, txt, ha="center", va="bottom", fontsize=8.5)
    ax.set_title(title, fontsize=10, fontweight="bold")
    ax.set_ylim(0, max(vals) * (1.3 if note else 1.18))

def cap_line(ax, ymax, x=2.45, ha="right"):
    ax.axhline(CAP, color="#C0392B", ls="--", lw=1)
    ax.text(x, CAP + 1, f"C = {CAP}", color="#C0392B", fontsize=8, ha=ha, va="bottom")
    ax.set_ylim(0, max(ymax * 1.18, CAP * 1.2))

# Figure 1: the four metrics on the largest network
sc = read("scalability.csv"); big = max(r["nodes"] for r in sc)
rows = [r for r in sc if r["nodes"] == big]; roads = int(rows[0]["roads"]); steps = int(rows[0]["steps"])
fig, axs = plt.subplots(2, 2, figsize=(7.2, 5.6))
bars(axs[0, 0], "max_load", rows, "(a) Maximum road load"); cap_line(axs[0, 0], max(r["max_load"] for r in rows))
bars(axs[0, 1], "avg_load", rows, "(b) Average road load", "{:.2f}")
bars(axs[1, 0], "variance", rows, "(c) Load variance", "{:.2f}")
sp = by_alg(rows)["Shortest Path"]["total_distance"]
bars(axs[1, 1], "total_distance", rows, "(d) Total travel distance",
     note=lambda a: None if a == "Shortest Path" else f"{(by_alg(rows)[a]['total_distance'] / sp - 1) * 100:+.0f}%")
axs[1, 1].yaxis.set_major_formatter(thousands)
fig.suptitle(f"Metric comparison, {int(big):,} nodes / {roads:,} roads", fontsize=10.5, fontweight="bold")
fig.tight_layout(rect=[0, 0, 1, 0.96]); fig.savefig(os.path.join(OUT, "fig1_metrics.png"), dpi=200); plt.close(fig)

# Figure 2: running time by network size (log scale)
sizes = sorted({r["nodes"] for r in sc if r["nodes"] >= MIN_NODES}); w = 0.26
fig, ax = plt.subplots(figsize=(7.2, 3.9))
for i, a in enumerate(ALGS):
    v = [next(r["time_ms"] for r in sc if r["nodes"] == n and r["algorithm"] == a) for n in sizes]
    b = ax.bar([k + (i - 1) * w for k in range(len(sizes))], v, w, label=a, color=COL[a])
    for r_, val in zip(b, v):
        ax.text(r_.get_x() + r_.get_width() / 2, val * 1.08, f"{val:,.0f}", ha="center", fontsize=7.5)
ax.set_yscale("log"); ax.set_xticks(range(len(sizes))); ax.set_xticklabels([f"{int(n):,}" for n in sizes])
ax.set_xlabel("Number of nodes"); ax.set_ylabel("Running time, ms (log scale, 3 steps)"); ax.legend(frameon=False, loc="upper left")
ax.set_ylim(top=ax.get_ylim()[1] * 2.5); ax.set_title("Running time by network size", fontsize=10.5, fontweight="bold")
fig.tight_layout(); fig.savefig(os.path.join(OUT, "fig2_runtime.png"), dpi=200); plt.close(fig)

# Figure 3: share of the total running time on the largest network
vals = [by_alg(rows)[a]["time_ms"] for a in ALGS]; tot = sum(vals)
fig, ax = plt.subplots(figsize=(6.4, 3.8))
wd, _ = ax.pie(vals, colors=[COL[a] for a in ALGS], startangle=90, counterclock=False, wedgeprops={"edgecolor": "white", "linewidth": 1.5})
ax.text(0, -0.35, f"{vals[2] / tot * 100:.1f}%", ha="center", va="center", color="white", fontsize=13, fontweight="bold")
ax.legend(wd, [f"{a}: {v:,.0f} ms ({v / tot * 100:.1f}%)" for a, v in zip(ALGS, vals)], loc="center left", bbox_to_anchor=(1, 0.5), frameon=False)
ax.set_title(f"Share of total running time, {int(big):,} nodes", fontsize=10.5, fontweight="bold")
fig.subplots_adjust(left=0.02, right=0.62, top=0.88, bottom=0.02); fig.savefig(os.path.join(OUT, "fig3_pie.png"), dpi=200); plt.close(fig)

# Figure 4: small example network
sm = read("small_example.csv")
fig, axs = plt.subplots(1, 2, figsize=(7.2, 3.4))
bars(axs[0], "max_load", sm, "(a) Peak road load"); cap_line(axs[0], max(r["max_load"] for r in sm), x=1.5, ha="center")
bars(axs[1], "variance", sm, "(b) Load variance")
fig.suptitle(f"Small example ({int(sm[0]['nodes'])} nodes, {int(sm[0]['roads'])} roads, {int(sm[0]['steps'])} time steps)", fontsize=10.5, fontweight="bold")
fig.tight_layout(rect=[0, 0, 1, 0.93]); fig.savefig(os.path.join(OUT, "fig4_small.png"), dpi=200); plt.close(fig)

# Figure 5: alpha sweep (all values in the CSV)
al = read("alpha_sweep.csv"); xl = [f"{r['alpha']:g}" for r in al]
fig, axs = plt.subplots(1, 3, figsize=(7.2, 3.2))
for ax, key, title in zip(axs, ["max_load", "variance", "total_distance"], ["Peak load", "Variance", "Total distance"]):
    v = [r[key] for r in al]
    ax.plot(range(len(v)), v, "o-", color="#2F6DB5")
    for i in (0, len(v) - 1):
        ax.annotate(f"{v[i]:,.0f}", (i, v[i]), textcoords="offset points", xytext=(0, 6), ha="center", fontsize=8)
    ax.set_xticks(range(len(v))); ax.set_xticklabels(xl, fontsize=7.5, rotation=45)
    ax.set_xlabel("α"); ax.set_title(title, fontsize=10, fontweight="bold")
    ax.set_ylim(0, max(v) * 1.2); ax.set_xlim(-0.6, len(v) - 0.4); ax.yaxis.set_major_formatter(thousands)
fig.suptitle(f"Balancing–distance trade-off (α sweep, {200} nodes)", fontsize=10.5, fontweight="bold")
fig.tight_layout(rect=[0, 0, 1, 0.92]); fig.savefig(os.path.join(OUT, "fig5_alpha.png"), dpi=200); plt.close(fig)
print("Figures written to", OUT)
