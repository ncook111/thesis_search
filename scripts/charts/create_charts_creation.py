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
    
    fig, ax = plt.subplots(figsize=(8, 5))
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

    parts = df["label"].str.split(",")
    df["scheme"] = parts.str[0]
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1


    all_cols = ["file_type", "file_size", "scheme", "compression", "real_time", "index_size", "data_size", "dataset"]
    return df.reindex(columns=all_cols)

def plot_compression_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index size ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_compression_by_file_type.png"
    
    data = df[df["file_size"] == file_size].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_compression.png"
        title = f"Index size ({file_size}, dataset_{dataset})"
 
    totals = data.groupby(["file_type", "scheme"])[["index_size", "data_size"]].sum()
    compression = (totals["index_size"] / totals["data_size"] * 100).unstack("scheme")
 
    save(out, compression, title, "Encoding Schemes", "file type", "% of input")
    
def plot_compression_by_file_size(df, file_type, folder_name, dataset=None):
    title = f"Index size ({file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_type}_compression_by_file_size.png"
    
    data = df[df["file_type"] == file_type].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_type}_compression.png"
        title = f"Index size ({file_type}, dataset_{dataset})"
 
    totals = data.groupby(["file_size", "scheme"], observed=True)[["index_size", "data_size"]].sum()
    compression = (totals["index_size"] / totals["data_size"] * 100).unstack("scheme")
 
    save(out, compression, title, "Encoding Schemes", "dataset size", "% of input")

def plot_compression_by_dataset(df, file_size, file_type, folder_name):
    title = f"Index size ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_compression_by_dataset.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)]

    compression = data.pivot(index="dataset", columns="scheme", values="compression") * 100

    save(out, compression, title, "Encoding Schemes", "dataset", "% of input")
    
def plot_throughput_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index creation throughput ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_throughput_by_file_type.png"
    
    data = df[df["file_size"] == file_size].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_throughput.png"
        title = f"Index creation throughput ({file_size}, dataset_{dataset})"

    totals = data.groupby(["file_type", "scheme"])[["data_size", "real_time"]].sum()
    throughput = (totals["data_size"] / 1000000 / totals["real_time"]).unstack("scheme")

    save(out, throughput, title, "Encoding Schemes", "file type", "MB/s")
    
def plot_throughput_by_file_size(df, file_type, folder_name, dataset=None):
    title = f"Index creation throughput ({file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_type}_throughput_by_file_size.png"
    
    data = df[df["file_type"] == file_type].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_type}_throughput.png"
        title = f"Index creation throughput ({file_type}, dataset_{dataset})"

    totals = data.groupby(["file_size", "scheme"], observed=True)[["data_size", "real_time"]].sum()
    throughput = (totals["data_size"] / 1000000 / totals["real_time"]).unstack("scheme")

    save(out, throughput, title, "Encoding Schemes", "dataset size", "MB/s")
    
def plot_throughput_by_dataset(df, file_size, file_type, folder_name):
    title = f"Index creation throughput ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_throughput_by_dataset.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)].copy()

    data["throughput"] = data["data_size"] / 1000000 / data["real_time"]
    throughput = data.pivot(index="dataset", columns="scheme", values="throughput")

    save(out, throughput, title, "Encoding Schemes", "dataset", "MB/s")

def plot_index_size_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index size ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_index_size_by_file_type.png"

    data = df[df["file_size"] == file_size]
    if dataset is not None:
        data = data[data["dataset"] == dataset]
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_index_size.png"
        title = f"Index size ({file_size}, dataset_{dataset})"

    index_size = (
        data.groupby(["file_type", "scheme"])["index_size"].sum() / 1_000_000
    ).unstack("scheme")

    save(out, index_size, title, "Encoding Schemes", "file type", "Index size (MB)")

if __name__ == "__main__":
    
    # Single Threaded Chart Creation
    df = load_results(f"{RESULTS_DIR}/creation/creation_single_threaded.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/creation_single_threaded", exist_ok=True)
    
    file_sizes = df["file_size"].unique()
    file_types = df["file_type"].unique()
    datasets = [None, 1, 2, 3, 4]
    
    for dataset in datasets:
        plot_compression_by_file_type(df, "small", "creation_single_threaded", dataset)
        plot_throughput_by_file_type(df, "small", "creation_single_threaded", dataset)
    
    plot_index_size_by_file_type(df, "small", "creation_single_threaded", None)

    for file_type in file_types:
        plot_compression_by_dataset(df, "small", file_type, "creation_single_threaded")
        plot_throughput_by_dataset(df, "small", file_type, "creation_single_threaded")



