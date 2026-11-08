#!/usr/bin/env python3
import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.ticker import ScalarFormatter

EXCLUDED_SCHEMES = ["pef", "fls"]

SCHEMES = {
    "gvb": "GVB + Delta", "pef": "PEF", "fls": "FLS",
    "s4_bpd4": "S4-BP128-D4", "s4_bpdm": "S4-BP128-DM",
    "s8_bpd4": "S8-BP128-D4", "s8_bpdm": "S8-BP128-DM",
    "s16_bpd4": "S16-BP128-D4", "s16_bpdm": "S16-BP128-DM",
    "gzip": "GZIP", "lz4": "LZ4"
}

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

plt.rcParams["axes.prop_cycle"] = plt.cycler(color=plt.cm.tab20.colors[::2] + plt.cm.tab20.colors[1::2])

def save(out, data, chart_title, legend_title, xlabel, ylabel, logy=False):
    if data.empty:
        print(f"{out}: no data")
        return
    
    fig, ax = plt.subplots(figsize=(8, 5))
    
    if logy:  
        data.plot.bar(ax=ax, rot=0, title=chart_title,
                         xlabel=xlabel, ylabel=ylabel, logy=True)
        ax.yaxis.set_major_formatter(ScalarFormatter())
    else:
        data.plot.bar(ax=ax, rot=0, title=chart_title,
                                 xlabel=xlabel, ylabel=ylabel)
        
    ax.legend(title=legend_title, loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    
    # Tabular output
    base = os.path.splitext(out)[0]
    table = data.round(3)
    table.to_latex(f"{base}.tex", float_format="%.2f", escape=True)
    
    print(f"wrote {out}")

def load_results(path):
    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    parts = df["label"].str.split(",")
    df["scheme"] = pd.Categorical(parts.str[0], categories=["lz4", "gzip", "gvb", "pef", "fls", "s4_bpd4", "s4_bpdm", "s8_bpd4", "s8_bpdm", "s16_bpd4", "s16_bpdm"], ordered=True)
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1

    all_cols = ["file_type", "file_size", "scheme", "index_size", "data_size", "real_time", "dataset", "peak_delta"]
    return df.reindex(columns=all_cols)


def plot_compression_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index Compression By File Type ({file_size}, all datasets)"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_compression_by_file_type.png"
    
    data = df[df["file_size"] == file_size].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_compression.png"
        title = f"Index Compression ({file_size}, dataset_{dataset})"
 
    data = data.assign(compression=data["index_size"] / data["data_size"])
    compression = (data.groupby(["file_type", "scheme"], observed=True)["compression"].mean() * 100).unstack("scheme")

    cols = [s for s in SCHEMES if s in compression.columns]
    cols += [c for c in compression.columns if c not in cols]
    compression = compression[cols].rename(columns=SCHEMES)
 
    save(out, compression, title, "Encoding Schemes", "File Type", "% of input")

def plot_throughput_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index Creation Throughput By File Type({file_size}, all datasets)"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_throughput_by_file_type.png"
    
    data = df[df["file_size"] == file_size].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_throughput.png"
        title = f"Index Creation Throughput ({file_size}, dataset_{dataset})"

    totals = data.groupby(["file_type", "scheme"], observed=True)[["data_size", "real_time"]].sum()
    throughput = (totals["data_size"] / 1000000 / totals["real_time"]).unstack("scheme")

    cols = [s for s in SCHEMES if s in throughput.columns]
    cols += [c for c in throughput.columns if c not in cols]
    throughput = throughput[cols].rename(columns=SCHEMES)

    save(out, throughput, title, "Encoding Schemes", "File Type", "MB/s")
    
def plot_index_size_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index Size By File Type ({file_size}, all datasets)"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_index_size_by_file_type.png"

    data = df[df["file_size"] == file_size]
    if dataset is not None:
        data = data[data["dataset"] == dataset]
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_index_size.png"
        title = f"Index Size ({file_size}, dataset_{dataset})"

    index_size = (
        data.groupby(["file_type", "scheme"], observed=True)["index_size"].sum() / 1000000
    ).unstack("scheme")

    cols = [s for s in SCHEMES if s in index_size.columns]
    cols += [c for c in index_size.columns if c not in cols]
    index_size = index_size[cols].rename(columns=SCHEMES)

    save(out, index_size, title, "Encoding Schemes", "File Type", "Index size (MB)")

def plot_memory_usage_by_file_type(df, file_size, folder_name, dataset=None):
    # Average of all datasets unless dataset set
    title = f"Peak Memory Usage By File Type ({file_size}, all datasets)"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_memory_by_file_type.png"

    data = df[df["file_size"] == file_size].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_memory.png"
        title = f"Peak Memory ({file_size}, dataset_{dataset})"

    data["peak_mb"] = data["peak_delta"] / 1000000

    memory = data.groupby(["file_type", "scheme"], observed=True)["peak_mb"].mean().unstack("scheme")

    cols = [s for s in SCHEMES if s in memory.columns]
    cols += [c for c in memory.columns if c not in cols]
    memory = memory[cols].rename(columns=SCHEMES)

    save(out, memory, title, "Encoding Schemes", "File Type", "Peak memory (MB)", True)
    
if __name__ == "__main__":
    # Grep Comparison Chart Creation
    os.makedirs(f"{RESULTS_DIR}/figures/creation_grep", exist_ok=True)

    df_0 = load_results(f"{RESULTS_DIR}/creation_grep/creation_grep.json")
    df_1 = load_results(f"{RESULTS_DIR}/creation_grep/creation_file_types.json")
    df = pd.concat([df_0, df_1], ignore_index=True)

    plot_index_size_by_file_type(df, "small", "creation_grep", None)
    plot_compression_by_file_type(df, "small", "creation_grep", None)
    plot_throughput_by_file_type(df, "small", "creation_grep", None)
    plot_memory_usage_by_file_type(df, "small", "creation_grep", None)