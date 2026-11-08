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

def load_results_grep(path):
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

def plot_latency_by_file_type(df, file_size folder_name):
    if dataset is None:
        ds_title, ds_file = "all datasets (mean)", "all_datasets"
    else:
        ds_title, ds_file = f"dataset {dataset}", f"dataset_{dataset}"

    title = f"Query Latency ({query_type}, {ds_title}, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_single_threaded/{query_type}_{ds_file}_{file_size}_{file_type}_latency_by_scheme.png"

    data = df[(df["query_type"] == query_type)
              & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if dataset is not None:
        data = data[data["dataset"] == dataset]

    latency = data.pivot_table(index="scheme", columns="stat", values="real_time",
                               aggfunc="mean", observed=True)
    latency = latency.reindex(columns=["median", "p95"]).dropna(axis=1, how="all")

    order = [s for s in SCHEMES if s in latency.index]
    order += [s for s in latency.index if s not in order]
    latency = latency.loc[order].rename(index=SCHEMES)

    save(out, latency, title, "Statistics", "Encoding Scheme", "Microseconds")
    
def plot_throughput_by_file_type(df, file_size, folder_name):
    stats = ["median"]
    for stat in stats:
        title = f"Query Throughput ({stat}, {query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/{query_type}_{stat}_{file_size}_{file_type}_throughput_by_dataset.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["query_type"] == query_type]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        throughput = data.pivot(index="dataset", columns="scheme", values="throughput")

        cols = [s for s in SCHEMES if s in throughput.columns]
        cols += [c for c in throughput.columns if c not in cols]
        throughput = throughput[cols].rename(columns=SCHEMES)

        save(out, throughput, title, "Encoding Schemes", "Datasets", "Queries/s")
    

def plot_memory_usage_by_file_type(df, file_size, folder_name):
    title = f"Median Peak Memory Usage During Search (all datasets, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_single_threaded/all_datasets_{file_size}_{file_type}_memory_by_query.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]

    memory = data.pivot_table(index="query_type", columns="scheme", values="peak_delta",
                              aggfunc="mean", observed=True) / (1024 ** 2)
    order = ["simple", "union", "intersection", "combination", "complex_combination"]
    memory = memory.reindex(index=[q for q in order if q in memory.index])

    cols = [s for s in SCHEMES if s in memory.columns]
    cols += [c for c in memory.columns if c not in cols]
    memory = memory[cols].rename(columns=SCHEMES)

    save(out, memory, title, "Encoding Scheme", "Query Type", "Peak Memory Increase (MB)")
    
if __name__ == "__main__":
    df = load_results_grep(f"{RESULTS_DIR}/search_grep/search_grep.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/search_single_threaded", exist_ok=True)
    

    plot_latency_by_file_type(df, "small", "search_grep")
    plot_throughput_by_file_type(df, "small", "search_grep")          
    plot_memory_usage_by_file_type(df, "small", "search_grep") 
