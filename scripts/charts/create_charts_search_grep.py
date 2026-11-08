#!/usr/bin/env python3
import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

EXCLUDED_SCHEMES = ["pef", "fls"]

SCHEMES = {
    "gvb": "GVB + Delta", "pef": "PEF", "fls": "FLS",
    "s4_bpd4": "S4-BP128-D4", "s4_bpdm": "S4-BP128-DM",
    "s8_bpd4": "S8-BP128-D4", "s8_bpdm": "S8-BP128-DM",
    "s16_bpd4": "S16-BP128-D4", "s16_bpdm": "S16-BP128-DM",
    "gzip": "GZIP", "lz4": "LZ4",
}

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

def save(out, data, chart_title, legend_title, xlabel, ylabel, zero_line=False):
    if data.empty:
        print(f"{out}: no data")
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    data.plot.bar(ax=ax, rot=0, title=chart_title,
                  xlabel=xlabel, ylabel=ylabel, legend=bool(legend_title))
    if legend_title:
        ax.legend(title=legend_title, loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0)

    if zero_line:
        ax.axhline(0, color="black", linewidth=0.8)

    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")

def load_results(path):
    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    parts = df["label"].str.split(",")
    df["scheme"] = parts.str[0]
    df["file_size"] = parts.str[1]
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1
    df["throughput"] = 1 / df["real_time"]  # one query per run -> queries/sec

    keys = ["scheme", "file_size", "file_type", "dataset"]
    g = df.groupby(keys)[["real_time", "throughput", "peak_delta"]]
    df = pd.concat([g.mean().assign(stat="mean"),
                    g.median().assign(stat="median")]).reset_index()

    df["scheme"] = pd.Categorical(df["scheme"], categories=["lz4", "gzip", "gvb", "pef", "fls", "s4_bpd4", "s4_bpdm", "s8_bpd4", "s8_bpdm", "s16_bpd4", "s16_bpdm"], ordered=True)
    df["file_size"] = pd.Categorical(df["file_size"], categories=["small", "medium", "large"], ordered=True)

    df = df[~df["scheme"].isin(EXCLUDED_SCHEMES)]

    all_cols = ["file_type", "file_size", "scheme", "stat", "real_time", "throughput",
                "index_size", "data_size", "dataset", "peak_delta"]
    return df.reindex(columns=all_cols)

STATS = ["mean", "median"]

def plot_latency_by_file_type(df, file_size, folder_name):
    for stat in STATS:
        title = f"Query Latency ({stat}, all datasets, {file_size})"
        out = f"{RESULTS_DIR}/figures/{folder_name}/{stat}_{file_size}_latency_by_file_type.png"

        data = df[(df["stat"] == stat) & (df["file_size"] == file_size)]

        latency = data.pivot_table(index="file_type", columns="scheme", values="real_time",
                                 aggfunc="mean", observed=True)
        latency.columns = latency.columns.astype(str)

        cols = [s for s in SCHEMES if s in latency.columns]
        cols += [c for c in latency.columns if c not in cols]
        latency = latency[cols].rename(columns=SCHEMES)

        save(out, latency, title, "Encoding Schemes", "File Type", "Seconds")

def plot_throughput_by_file_type(df, file_size, folder_name):
    for stat in STATS:
        title = f"Query Throughput ({stat}, all datasets, {file_size})"
        out = f"{RESULTS_DIR}/figures/{folder_name}/{stat}_{file_size}_throughput_by_file_type.png"

        data = df[(df["stat"] == stat) & (df["file_size"] == file_size)]

        throughput = data.pivot_table(index="file_type", columns="scheme", values="throughput",
                                 aggfunc="mean", observed=True)
        throughput.columns = throughput.columns.astype(str)

        cols = [s for s in SCHEMES if s in throughput.columns]
        cols += [c for c in throughput.columns if c not in cols]
        throughput = throughput[cols].rename(columns=SCHEMES)

        save(out, throughput, title, "Encoding Schemes", "File Type", "Queries/s")

def plot_memory_usage_by_file_type(df, file_size, folder_name):
    for stat in STATS:
        title = f"{stat.capitalize()} Peak Memory Usage During Search (all datasets, {file_size})"
        out = f"{RESULTS_DIR}/figures/{folder_name}/{stat}_{file_size}_memory_by_file_type.png"

        data = df[(df["stat"] == stat) & (df["file_size"] == file_size)]

        memory = data.pivot_table(index="file_type", columns="scheme", values="peak_delta",
                                 aggfunc="mean", observed=True) / 1000000
        memory.columns = memory.columns.astype(str)

        cols = [s for s in SCHEMES if s in memory.columns]
        cols += [c for c in memory.columns if c not in cols]
        memory = memory[cols].rename(columns=SCHEMES)

        save(out, memory, title, "Encoding Schemes", "File Type", "Peak Memory Increase (MB)")

if __name__ == "__main__":
    df_0 = load_results(f"{RESULTS_DIR}/search_grep/search_grep.json")
    df_1 = load_results(f"{RESULTS_DIR}/search_grep/search_file_type_comparisons.json")
    df = pd.concat([df_0, df_1], ignore_index=True)
    
    os.makedirs(f"{RESULTS_DIR}/figures/search_grep", exist_ok=True)


    plot_latency_by_file_type(df, "small", "search_grep")
    plot_throughput_by_file_type(df, "small", "search_grep")          
    plot_memory_usage_by_file_type(df, "small", "search_grep")