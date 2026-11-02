#!/usr/bin/env python3
import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

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
        ax.legend(title=legend_title)

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

    df = df[df["aggregate_name"].isin(["mean", "median", "stddev", "cv", "p95"])].copy()
    
    parts = df["label"].str.split(",")
    df["scheme"] = pd.Categorical(parts.str[0], categories=["gvb", "pef", "fls", "s4_bpd4", "s4_bpdm", "s8_bpd4", "s8_bpdm", "s16_bpd4", "s16_bpdm"], ordered=True)
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1
    df["query_type"] = df["name"].str.split("/").str[3]

    df = df.rename(columns={"aggregate_name": "stat"})

    timed = df["stat"].isin(["mean", "median", "p95"])
    df["throughput"] = df["items_per_second"].where(timed)

    all_cols = ["file_type", "file_size", "scheme", "stat", "real_time", "throughput",
                "index_size", "data_size", "dataset", "query_type"]
    return df.reindex(columns=all_cols)

def plot_latency_by_dataset(df, query_type, file_type, file_size):
    stats = ["median", "p95"]
    for stat in stats:
        title = f"Query Latency ({stat}, {query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search/{query_type}_{stat}_{file_size}_{file_type}_latency_by_dataset.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["query_type"] == query_type]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        latency = data.pivot(index="dataset", columns="scheme", values="real_time")

        save(out, latency, title, "Encoding Schemes", "dataset", "microseconds")

def plot_latency_by_query_type(df, dataset, file_type, file_size):
    stats = ["median", "p95"]
    for stat in stats:
        title = f"Query Latency ({stat}, {dataset}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search/dataset_{dataset}_{stat}_{file_size}_{file_type}_latency_by_query.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["dataset"] == dataset] 
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        latency = data.pivot(index="query_type", columns="scheme", values="real_time")
        save(out, latency, title, "Encoding Schemes", "query type", "microseconds")

def plot_latency_by_scheme(df, query_type, dataset, file_type, file_size):
    if dataset is None:
        ds_title, ds_file = "all datasets (mean)", "all_datasets"
    else:
        ds_title, ds_file = f"dataset {dataset}", f"dataset_{dataset}"

    title = f"Query Latency ({query_type}, {ds_title}, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search/{query_type}_{ds_file}_{file_size}_{file_type}_latency_by_scheme.png"

    data = df[(df["query_type"] == query_type)
              & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if dataset is not None:
        data = data[data["dataset"] == dataset]

    latency = data.pivot_table(index="scheme", columns="stat", values="real_time",
                               aggfunc="mean", observed=True)
    latency = latency.reindex(columns=["median", "p95"]).dropna(axis=1, how="all")

    save(out, latency, title, "Statistics", "encoding scheme", "microseconds")
    
def plot_throughput_by_dataset(df, query_type, file_type, file_size):
    stats = ["median"]
    for stat in stats:
        title = f"Query Throughput ({stat}, {query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search/{query_type}_{stat}_{file_size}_{file_type}_throughput_by_dataset.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["query_type"] == query_type]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        throughput = data.pivot(index="dataset", columns="scheme", values="throughput")

        save(out, throughput, title, "Encoding Schemes", "encoding scheme", "queries/s")
    
def plot_throughput_by_query_type(df, dataset, file_type, file_size):
    stats = ["median"]
    for stat in stats:
        title = f"Query Throughput ({stat}, dataset {dataset}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search/dataset_{dataset}_{stat}_{file_size}_{file_type}_throughput_by_query.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["dataset"] == dataset]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        throughput = data.pivot(index="query_type", columns="scheme", values="throughput")
        save(out, throughput, title, "Encoding Schemes", "queries/s")
    
def plot_throughput_increase_by_scheme(df, query_type, file_type, file_size, baseline="gvb"):
    if query_type is not None:
        title = f"Median extra throughput vs {baseline} ({query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search/{query_type}_{file_size}_{file_type}_extra_throughput_vs_{baseline}.png"
    else:
        title = f"Median extra throughput vs {baseline} ({file_size}, {file_type}, all queries)"
        out = f"{RESULTS_DIR}/figures/search/all_queries_{file_size}_{file_type}_extra_throughput_vs_{baseline}.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if query_type is not None:
        data = data[data["query_type"] == query_type]
        throughput = data.pivot(index="dataset", columns="scheme", values="throughput")
    else:
        throughput = data.pivot(index=["dataset", "query_type"], columns="scheme", values="throughput")

    if throughput.empty or baseline not in throughput.columns:
        return

    base = throughput[baseline]
    extra = throughput.sub(base, axis=0)   # queries/s gained over baseline, per cell
    avg = extra.mean().drop(baseline).to_frame("median")

    save(out, avg, title, None, "Encoding Schemes", f"extra queries/s vs {baseline}", True)

def plot_latency_decrease_by_scheme(df, query_type, file_type, file_size, baseline="gvb"):
    if query_type is not None:
        title = f"Latency decrease vs {baseline} ({query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search/{query_type}_{file_size}_{file_type}_decrease_vs_{baseline}.png"
    else:
        title = f"Latency decrease vs {baseline} ({file_size}, {file_type}, all queries)"
        out = f"{RESULTS_DIR}/figures/search/all_queries_{file_size}_{file_type}_decrease_vs_{baseline}.png"

    results = {}
    for stat in ["median", "p95"]:
        data = df[(df["stat"] == stat) & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
        if query_type is not None:
            data = data[data["query_type"] == query_type]
            latency = data.pivot(index="dataset", columns="scheme", values="real_time")
        else:
            latency = data.pivot(index=["dataset", "query_type"], columns="scheme", values="real_time")

        if latency.empty or baseline not in latency.columns:
            continue

        base = latency[baseline]
        decrease = latency.rsub(base, axis=0).div(base, axis=0) * 100
        results[stat] = decrease.mean().drop(baseline)

    avg = pd.DataFrame(results)   # rows = schemes, columns = statistics
    save(out, avg, title, "Statistic", "encoding scheme", f"% lower latency than {baseline}", True)

if __name__ == "__main__":
    df = load_results(f"{RESULTS_DIR}/search/search.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/search", exist_ok=True)
    
    file_sizes = df["file_size"].unique()
    file_types = df["file_type"].unique()
    datasets = df["dataset"].unique()
    query_types = [None, "simple", "union", "intersection", "combination", "complex_combination"]

    for query in query_types:
        for file_type in file_types:
            for file_size in file_sizes:
                #plot_latency_decrease_by_scheme(df, None, file_type, file_size)
                plot_throughput_increase_by_scheme(df, query, file_type, file_size)