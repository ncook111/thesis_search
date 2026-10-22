#!/usr/bin/env python3
import json
import os

import matplotlib.pyplot as plt
import pandas as pd

def load_results(path):
    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    parts = df["label"].str.split(",")
    df["scheme"] = parts.str[0]
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1
    
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)

    all_cols = ["file_type", "file_size", "scheme", "compression", "real_time", "index_size", "data_size", "dataset"]
    return df.reindex(columns=all_cols)

def create_chart_compression_by_file_type(df, file_type, dataset=None):
    title = f"Index size ({file_type})"
    out = f"../results/figures/creation/{file_type}_compression.png"
    sub = df[df["file_type"] == file_type]
    if dataset is not None:
        sub = sub[sub["dataset"] == dataset]
        out = f"../results/figures/creation/dataset_{dataset}_{file_type}_compression.png"
        title = f"Index size ({file_type}, dataset_{dataset})"
 
    totals = sub.groupby(["file_size", "scheme"], observed=True)[["index_size", "data_size"]].sum()
    compression = (totals["index_size"] / totals["data_size"] * 100).unstack("scheme")
 
    fig, ax = plt.subplots(figsize=(8, 5))
    compression.plot.bar(ax=ax, rot=0, title=f"Index size ({file_type})",
                         xlabel="dataset size", ylabel="% of input")
    ax.legend(title="Encoding Schemes")
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    
def create_chart_throughput_by_file_type(df, file_type, dataset=None):
    title = f"Index creation throughput ({file_type})"
    out = f"../results/figures/creation/{file_type}_throughput.png"
    sub = df[df["file_type"] == file_type]
    if dataset is not None:
        sub = sub[sub["dataset"] == dataset]
        out = f"../results/figures/creation/dataset_{dataset}_{file_type}_throughput.png"
        title = title = f"Index creation throughput ({file_type}, dataset_{dataset})"

    totals = sub.groupby(["file_size", "scheme"], observed=True)[["data_size", "real_time"]].sum()
    throughput = (totals["data_size"] / 1000000 / totals["real_time"]).unstack("scheme")

    fig, ax = plt.subplots(figsize=(8, 5))
    throughput.plot.bar(ax=ax, rot=0, title=title,
                        xlabel="dataset size", ylabel="MB/s")
    ax.legend(title="Encoding Schemes")
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    


if __name__ == "__main__":
    df = load_results("../results/creation/creation.json")
    
    os.makedirs("../results/figures/creation", exist_ok=True)
    
    create_chart_compression_by_file_type(df, "csv")
    create_chart_compression_by_file_type(df, "csv", 1)
    create_chart_compression_by_file_type(df, "csv", 2)
    create_chart_compression_by_file_type(df, "csv", 3)
    create_chart_compression_by_file_type(df, "csv", 4)
    create_chart_compression_by_file_type(df, "csv", 5)
    create_chart_throughput_by_file_type(df, "csv")
    create_chart_throughput_by_file_type(df, "csv", 1)
    create_chart_throughput_by_file_type(df, "csv", 2)
    create_chart_throughput_by_file_type(df, "csv", 3)
    create_chart_throughput_by_file_type(df, "csv", 4)
    create_chart_throughput_by_file_type(df, "csv", 5)



