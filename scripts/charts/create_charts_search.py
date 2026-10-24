#!/usr/bin/env python3
import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

def save(out, data, chart_title, legend_title, xlabel, ylabel):
    if data.empty:
        print(f"{out}: no data")
        return
    
    fig, ax = plt.subplots(figsize=(10, 5))
    data.plot.bar(ax=ax, rot=0, title=chart_title,
                         xlabel=xlabel, ylabel=ylabel)
    ax.legend(title=legend_title)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")

def load_results(path):
    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    df = df[df["aggregate_name"].isin(["mean", "median", "stddev", "cv", "p90", "p99"])].copy()
    
    parts = df["label"].str.split(",")
    df["scheme"] = parts.str[0]
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1
    df["query_type"] = df["name"].str.split("/").str[3]

    df = df.rename(columns={"aggregate_name": "stat"})

    all_cols = ["file_type", "file_size", "scheme", "stat", "real_time", "index_size", "data_size", "dataset", "query_type"]
    return df.reindex(columns=all_cols)

def plot_latency_by_dataset(df, query_type, file_type, file_size):
    stats = ["median", "p90", "p99"]
    for stat in stats:
        title = f"Query Latency ({stat}, {query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search/{query_type}_{stat}_{file_size}_{file_type}_latency_by_dataset.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["query_type"] == query_type]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        latency = data.pivot(index="dataset", columns="scheme", values="real_time")

        save(out, latency, title, "Encoding Schemes", "dataset", "microseconds")

def plot_latency_by_query_type(df, dataset, file_type, file_size):
    stats = ["median", "p90", "p99"]
    for stat in stats:
        title = f"Query Latency ({stat}, {dataset}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search/dataset_{dataset}_{stat}_{file_size}_{file_type}_latency_by_query.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["dataset"] == dataset] 
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        latency = data.pivot(index="query_type", columns="scheme", values="real_time")
        save(out, latency, title, "Encoding Schemes", "query type", "microseconds")

def plot_latency_by_scheme(df, query_type, dataset, file_type, file_size):
    title = f"Query Latency ({query_type}, dataset {dataset}, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search/{query_type}_dataset_{dataset}_{file_size}_{file_type}_latency_by_scheme.png"

    data = df[(df["query_type"] == query_type) & (df["dataset"] == dataset)
              & (df["file_size"] == file_size) & (df["file_type"] == file_type)]

    latency = data.pivot(index="scheme", columns="stat", values="real_time")
    latency = latency.reindex(columns=["median", "p90", "p99"]).dropna(axis=1, how="all")

    save(out, latency, title, "Statistic", "encoding scheme", "microseconds")

       
if __name__ == "__main__":
    df = load_results(f"{RESULTS_DIR}/search/search.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/search", exist_ok=True)
    
    file_sizes = df["file_size"].unique()
    file_types = df["file_type"].unique()
    datasets = df["dataset"].unique()
    query_types = ["simple", "union", "intersection", "combination", "complex_combination"]

    for query in query_types:
        for dataset in datasets:
            for type in file_types:
                for size in file_sizes:
                    plot_latency_by_scheme(df, query, dataset, type, size)


