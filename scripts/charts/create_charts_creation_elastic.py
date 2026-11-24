#!/usr/bin/env python3
import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.ticker import ScalarFormatter

SCHEMES = {
    "gvb": "GVB + Delta", "pef": "PEF", "fls": "FLS",
    "s4_bpd4": "S4-BP128-D4", "s4_bpdm": "S4-BP128-DM",
    "s8_bpd4": "S8-BP128-D4", "s8_bpdm": "S8-BP128-DM",
    "s16_bpd4": "S16-BP128-D4", "s16_bpdm": "S16-BP128-DM",
    "elastic": "ELASTIC"
}

FILE_TYPES = { "csv" : "CSV", "json" : "JSON", "logfmt" : "LOGFMT" }

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

FONT_SIZE = 14

plt.rcParams["axes.prop_cycle"] = plt.cycler(color=plt.cm.tab20.colors[::2] + plt.cm.tab20.colors[1::2])

def save_chart(out, data, chart_title, legend_title, xlabel, ylabel, logy=False):
    if data.empty:
        print(f"{out}: no data")
        return
    
    with plt.rc_context({"font.size": FONT_SIZE}):
        fig, ax = plt.subplots(figsize=(8, 5))
        
        if logy:  
            data.plot.bar(ax=ax, rot=0,
                            xlabel=xlabel, ylabel=ylabel, logy=True)
            ax.yaxis.set_major_formatter(ScalarFormatter())
        else:
            data.plot.bar(ax=ax, rot=0,
                                    xlabel=xlabel, ylabel=ylabel)
        ax.set_title(chart_title)   
        ax.legend(title=legend_title, loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0, frameon=False)
        ax.grid(alpha=0.3)
        fig.tight_layout()
        fig.savefig(out, dpi=300)
        plt.close(fig)
        print(f"wrote chart {out}")

def save_table(out, data, row_name, col_name):
    # Tabular output
    base = os.path.splitext(out)[0]
    table = data.round(3)
    latex_table = table.T.rename_axis(row_name).reset_index()
    latex_table.columns = pd.MultiIndex.from_tuples(
        [("", row_name)] + [(col_name, label) for label in table.index]
    )
    latex_table.to_latex(f"{base}.tex", index=False, float_format="%.2f",
                         escape=True, multicolumn_format="c")
    print(f"wrote table {base}.tex")
    
def load_results(path):
    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    parts = df["label"].str.split(",")
    
    scheme_order = list(SCHEMES)
    df["scheme"] = (
        pd.Categorical(parts.str[0], categories=scheme_order, ordered=True)
          .rename_categories(SCHEMES) 
    )
          
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)
        
    df["dataset"] = parts.str[2].astype(int) + 1
    
    to_seconds = {"s": 1, "ms": 1e3, "us": 1e6, "ns": 1e9}
    unit = df["time_unit"] if "time_unit" in df else pd.Series("s", index=df.index)
    df["real_time"] = df["real_time"] / unit.fillna("s").map(to_seconds)

    all_cols = ["file_type", "file_size", "scheme", "index_size", "data_size", "real_time", "dataset", "peak_delta"]
    return df.reindex(columns=all_cols)

def plot_compression_by_dataset(df, file_size, folder_name):
    title = f"Index Compression By Dataset ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_compression_by_dataset.png"

    data = df[df["file_size"] == file_size].copy()
    data = data.assign(compression=data["index_size"] / data["data_size"])
    compression = (data.groupby(["dataset", "scheme"], observed=True)["compression"].mean() * 100).unstack("scheme")

    save_chart(out, compression, title, "Encoding Schemes", "Dataset", "% of input", True)
    save_table(out, compression, "Scheme", "Dataset Number")

def plot_throughput_by_dataset(df, file_size, folder_name):
    title = f"Index Creation Throughput By Dataset ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_throughput_by_dataset.png"

    data = df[df["file_size"] == file_size]
    totals = data.groupby(["dataset", "scheme"], observed=True)[["data_size", "real_time"]].sum()
    throughput = (totals["data_size"] / 1000000 / (totals["real_time"])).unstack("scheme")

    save_chart(out, throughput, title, "Encoding Schemes", "Dataset", "MB/s")
    save_table(out, throughput, "Scheme", "Dataset Number")

def plot_index_size_by_dataset(df, file_size, folder_name):
    title = f"Index Size By Dataset ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_index_size_by_dataset.png"

    data = df[df["file_size"] == file_size]
    index_size = (
        data.groupby(["dataset", "scheme"], observed=True)["index_size"].sum() / 1000000
    ).unstack("scheme")

    save_chart(out, index_size, title, "Encoding Schemes", "Dataset", "Index size (MB)", True)
    save_table(out, index_size, "Scheme", "Dataset Number")

def plot_memory_usage_by_dataset(df, file_size, folder_name):
    title = f"Peak Memory Usage By Dataset ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_memory_by_dataset.png"

    data = df[df["file_size"] == file_size].copy()
    data["peak_mb"] = data["peak_delta"] / 1000000
    memory = data.groupby(["dataset", "scheme"], observed=True)["peak_mb"].mean().unstack("scheme")

    save_chart(out, memory, title, "Encoding Schemes", "Dataset", "Peak Memory (MB)", True)
    save_table(out, memory, "Scheme", "Dataset Number")
    
if __name__ == "__main__":
    # Elastic Search Comparison Chart Creation
    os.makedirs(f"{RESULTS_DIR}/figures/creation_elastic", exist_ok=True)

    df_0 = load_results(f"{RESULTS_DIR}/creation_elastic/creation_elastic.json")
    df_1 = load_results(f"{RESULTS_DIR}/creation_elastic/creation_simdindex.json")
    df = pd.concat([df_0, df_1], ignore_index=True)

    plot_index_size_by_dataset(df, "small", "creation_elastic")
    plot_compression_by_dataset(df, "small", "creation_elastic")
    plot_throughput_by_dataset(df, "small", "creation_elastic")
    plot_memory_usage_by_dataset(df, "small", "creation_elastic")
    plot_index_size_by_dataset(df, "medium", "creation_elastic")
    plot_compression_by_dataset(df, "medium", "creation_elastic")
    plot_throughput_by_dataset(df, "medium", "creation_elastic")
    plot_memory_usage_by_dataset(df, "medium", "creation_elastic")
    plot_index_size_by_dataset(df, "large", "creation_elastic")
    plot_compression_by_dataset(df, "large", "creation_elastic")
    plot_throughput_by_dataset(df, "large", "creation_elastic")
    plot_memory_usage_by_dataset(df, "large", "creation_elastic")