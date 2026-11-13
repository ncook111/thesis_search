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
    "gzip": "GZIP", "lz4": "LZ4"
}

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

def save_chart(out, data, chart_title, legend_title, xlabel, ylabel):
    if data.empty:
        print(f"{out}: no data")
        return
    
    fig, ax = plt.subplots(figsize=(8, 5))
    data.plot.bar(ax=ax, rot=0, title=chart_title,
                         xlabel=xlabel, ylabel=ylabel)
    ax.legend(title=legend_title, loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    
    print(f"wrote {out}")
    
def save_chart_no_legend(out, data, chart_title, xlabel, ylabel):
    if data.empty:
        print(f"{out}: no data")
        return
    
    fig, ax = plt.subplots(figsize=(10, 5))
    data.plot.bar(ax=ax, rot=0, title=chart_title,
                         xlabel=xlabel, ylabel=ylabel, legend=False)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    
    print(f"wrote {out}")
    
def save_table(out, data, row_name, col_name):
    # Tabular output
    if data.empty:
        print(f"{out}: no data")
        return

    base = os.path.splitext(out)[0]
    table = data.round(3)
    if isinstance(table, pd.Series):
        # If table contains two columns
        latex_table = table.rename(col_name).rename_axis(row_name).reset_index()
    else:
        # If table contains more than two columns
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
    
    df["scheme"] = pd.Categorical(
        parts.str[0],
        categories=["gvb", "pef", "fls", "s4_bpd4", "s4_bpdm", "s8_bpd4", "s8_bpdm", "s16_bpd4", "s16_bpdm"],
        ordered=True,
    ).rename_categories(SCHEMES)
    
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1
    df["partition_size"] = parts.str[4].astype(int)
    df["thread_count"] = parts.str[5].astype(int)

    all_cols = ["file_type", "file_size", "scheme", "real_time", "index_size", "data_size", "dataset", "peak_delta", "partition_size", "thread_count"]
    return df.reindex(columns=all_cols)

def plot_compression_by_scheme(df, file_size, file_type, folder_name, dataset=None):
    title = f"Index Compression By Encoding Scheme ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_compression_by_scheme.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if dataset is not None:
        data = data[data["dataset"] == dataset]
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_{file_type}_compression_by_scheme.png"
        title = f"Index Compression By Encoding Scheme ({file_size}, {file_type}, dataset_{dataset})"

    data = data.assign(compression=data["index_size"] / data["data_size"])
    compression = data.groupby("scheme")["compression"].mean() * 100

    save_chart_no_legend(out, compression, title, "Encoding Scheme", "% of input")
    save_table(out, compression, "Scheme", "% of input")
         
def plot_compression_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index Compression By File Type ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_compression_by_file_type.png"
    
    data = df[df["file_size"] == file_size].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_compression.png"
        title = f"Index Compression ({file_size}, dataset_{dataset})"
 
    data = data.assign(compression=data["index_size"] / data["data_size"])
    compression = (data.groupby(["file_type", "scheme"])["compression"].mean() * 100).unstack("scheme")
 
    save_chart(out, compression, title, "Encoding Schemes", "File Type", "% of input")
    save_table(out, compression, "Scheme", "File Type")
    
def plot_compression_by_file_size(df, file_type, folder_name, dataset=None):
    title = f"Index Compression By File Size ({file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_type}_compression_by_file_size.png"
    
    data = df[df["file_type"] == file_type].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_type}_compression.png"
        title = f"Index Compression ({file_type}, dataset_{dataset})"
 
    data = data.assign(compression=data["index_size"] / data["data_size"])
    compression = (data.groupby(["file_size", "scheme"], observed=True)["compression"].mean() * 100).unstack("scheme")
 
    save_chart(out, compression, title, "Encoding Schemes", "Dataset Size", "% of input")
    save_table(out, compression, "Scheme", "Dataset Size")

def plot_compression_by_dataset(df, file_size, file_type, folder_name):
    title = f"Index Compression By Dataset ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_compression_by_dataset.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)]

    data = data.assign(compression=data["index_size"] / data["data_size"])
    compression = data.pivot(index="dataset", columns="scheme", values="compression").sort_index(axis=1) * 100

    save_chart(out, compression, title, "Encoding Schemes", "Dataset", "% of input")
    save_table(out, compression, "Scheme", "Dataset")
    
def plot_throughput_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index Creation Throughput By File Type({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_throughput_by_file_type.png"
    
    data = df[df["file_size"] == file_size].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_throughput.png"
        title = f"Index Creation Throughput ({file_size}, dataset_{dataset})"

    totals = data.groupby(["file_type", "scheme"])[["data_size", "real_time"]].sum()
    throughput = (totals["data_size"] / 1000000 / totals["real_time"]).unstack("scheme")

    save_chart(out, throughput, title, "Encoding Schemes", "File Type", "MB/s")
    save_table(out, throughput, "Scheme", "File Type")
    
def plot_throughput_by_file_size(df, file_type, folder_name, dataset=None):
    title = f"Index Creation Throughput By File Size({file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_type}_throughput_by_file_size.png"
    
    data = df[df["file_type"] == file_type].copy()
    if dataset is not None:
        data = data[data["dataset"] == dataset].copy()
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_type}_throughput.png"
        title = f"Index Creation Throughput ({file_type}, dataset_{dataset})"

    totals = data.groupby(["file_size", "scheme"], observed=True)[["data_size", "real_time"]].sum()
    throughput = (totals["data_size"] / 1000000 / totals["real_time"]).unstack("scheme")

    save_chart(out, throughput, title, "Encoding Schemes", "Dataset Size", "MB/s")
    save_table(out, throughput, "Scheme", "Dataset Size")
    
def plot_throughput_by_dataset(df, file_size, file_type, folder_name):
    title = f"Index Creation Throughput By Dataset ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_throughput_by_dataset.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)].copy()

    data["throughput"] = data["data_size"] / 1000000 / data["real_time"]
    throughput = data.pivot(index="dataset", columns="scheme", values="throughput").sort_index(axis=1)

    save_chart(out, throughput, title, "Encoding Schemes", "Dataset", "MB/s")
    save_table(out, throughput, "Scheme", "Dataset")

def plot_index_size_by_file_type(df, file_size, folder_name, dataset=None):
    title = f"Index Size By File Type ({file_size})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_index_size_by_file_type.png"

    data = df[df["file_size"] == file_size]
    if dataset is not None:
        data = data[data["dataset"] == dataset]
        out = f"{RESULTS_DIR}/figures/{folder_name}/dataset_{dataset}_{file_size}_index_size.png"
        title = f"Index Size ({file_size}, dataset_{dataset})"

    index_size = (
        data.groupby(["file_type", "scheme"])["index_size"].sum() / 1e6
    ).unstack("scheme")

    save_chart(out, index_size, title, "Encoding Schemes", "File Type", "Index size (MB)")
    save_table(out, index_size, "Scheme", "File Type")

def plot_throughput_by_partition_size(df, file_size, file_type, folder_name, scheme=None):
    # Average of all datasets, average of all schemes unless scheme set
    if scheme is None:
        tag = "all_schemes"
    else:
        tag = scheme
    title = f"Index Creation Throughput By Partition Size ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_{tag}_throughput_by_partition_size.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)].copy()
    if scheme is not None:
        data = data[data["scheme"] == SCHEMES.get(scheme, scheme)]
    data["throughput"] = data["data_size"] / 1e6 / data["real_time"]

    throughput = data.groupby("partition_size")["throughput"].mean()

    if scheme is None:
        save_chart_no_legend(out, throughput, title, "Partition Size (MB)", "MB/s")
    else:
        save_chart(out, throughput.to_frame(SCHEMES.get(scheme, scheme)), title, "Encoding Schemes", "Partition Size (MB)", "MB/s")
    save_table(out, throughput, "Partition Size (MB)", "MB/s")


def plot_compression_by_partition_size(df, file_size, file_type, folder_name, scheme=None):
    # Average of all datasets, average of all schemes unless scheme set
    if scheme is None:
        tag = "all_schemes"
    else:
        tag = scheme
    title = f"Index Compression By Partition Size ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_{tag}_compression_by_partition_size.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)].copy()
    if scheme is not None:
        data = data[data["scheme"] == SCHEMES.get(scheme, scheme)]
    data["compression_pct"] = data["index_size"] / data["data_size"] * 100

    compression = data.groupby("partition_size")["compression_pct"].mean()

    if scheme is None:
        save_chart_no_legend(out, compression, title, "Partition Size (MB)", "% of input")
    else:
        save_chart(out, compression.to_frame(SCHEMES.get(scheme, scheme)), title, "Encoding Schemes", "Partition Size (MB)", "% of input")
    save_table(out, compression, "Partition Size (MB)", "% of input")

        

def plot_memory_usage_by_partition_size(df, file_size, file_type, folder_name, scheme=None):
    # Average of all datasets, average of all schemes unless scheme set
    if scheme is None:
        tag = "all_schemes"
    else:
        tag = scheme
    title = f"Peak Memory by Partition Size ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_{tag}_memory_by_partition_size.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)].copy()
    if scheme is not None:
        data = data[data["scheme"] == SCHEMES.get(scheme, scheme)]
    data["peak_mb"] = data["peak_delta"] / 1e6

    memory = data.groupby("partition_size")["peak_mb"].mean()

    if scheme is None:
        save_chart_no_legend(out, memory, title, "Partition Size (MB)", "Peak memory (MB)")
    else:
        save_chart(out, memory.to_frame(SCHEMES.get(scheme, scheme)), title, "Encoding Schemes", "Partition Size (MB)", "Peak memory (MB)")
    save_table(out, memory, "Partition Size (MB)", "Peak memory (MB)")


def plot_memory_usage_by_scheme(df, file_size, file_type, folder_name):
    # Average of all datasets
    title = f"Peak Memory By Scheme ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_memory_by_scheme.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)].copy()
    data["peak_mb"] = data["peak_delta"] / 1e6

    memory = data.groupby("scheme")["peak_mb"].mean()


    save_chart_no_legend(out, memory, title, "Encoding Scheme", "Peak memory (MB)")
    save_table(out, memory, "Scheme", "Peak memory (MB)")
        
def plot_throughput_by_thread_count(df, file_size, file_type, folder_name, scheme=None):
    # Average of all datasets, average of all schemes unless scheme set
    tag = scheme or "all_schemes"
    title = f"Index Creation Throughput By Thread Count ({file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_{tag}_throughput_by_thread_count.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)].copy()
    data["throughput"] = data["data_size"] / 1e6 / data["real_time"]

    if scheme is None:
        throughput = data.groupby(["thread_count", "scheme"])["throughput"].mean().unstack("scheme").sort_index()
        save_chart(out, throughput, title, "Encoding Schemes", "Thread Count", "MB/s")
        save_table(out, throughput, "Scheme", "Thread Count")
        return

    if scheme == "average":
        title = f"Index Creation Throughput By Thread Count ({file_size}, {file_type}, average of schemes)"
    else:
        data = data[data["scheme"] == SCHEMES.get(scheme, scheme)]
        title = f"Index Creation Throughput By Thread Count ({file_size}, {file_type}, {SCHEMES.get(scheme, scheme)})"

    throughput = data.groupby("thread_count")["throughput"].mean().sort_index()
    save_chart_no_legend(out, throughput, title, "Thread Count", "MB/s")
    save_table(out, throughput, "Thread Count", "MB/s")


def plot_memory_usage_by_thread_count(df, file_size, file_type, folder_name, scheme=None):
    # Average of all datasets, average of all schemes unless scheme set
    if scheme is None:
        tag = "all_schemes"
        title = f"Peak Memory By Thread Count ({file_size}, {file_type})"
    else:
        tag = scheme
        title = f"Peak Memory By Thread Count ({file_size}, {file_type}, {SCHEMES.get(scheme, scheme)})"
    
    out = f"{RESULTS_DIR}/figures/{folder_name}/{file_size}_{file_type}_{tag}_memory_by_thread_count.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)].copy()
    if scheme is not None:
        data = data[data["scheme"] == SCHEMES.get(scheme, scheme)]
    data = data.dropna(subset=["thread_count"])
    data["peak_mb"] = data["peak_delta"] / 1e6

    memory = data.groupby("thread_count")["peak_mb"].mean().sort_index()

    save_chart_no_legend(out, memory, title, "Thread Count", "Peak memory (MB)")
    save_table(out, memory, "Thread Count", "Peak memory (MB)")
    
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
        plot_index_size_by_file_type(df, "small", "creation_single_threaded", dataset)

    for file_type in file_types:
        plot_compression_by_dataset(df, "small", file_type, "creation_single_threaded")
        plot_throughput_by_dataset(df, "small", file_type, "creation_single_threaded")

    # Partition Size Chart Creation
    df = load_results(f"{RESULTS_DIR}/creation/creation_partitioned.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/creation_partitioned", exist_ok=True)
    
    partition_sizes = sorted(df["partition_size"].dropna().unique())
    schemes = df["scheme"].unique()

    plot_throughput_by_partition_size(df, "medium", "csv", "creation_partitioned")
    plot_compression_by_partition_size(df, "medium", "csv", "creation_partitioned")
    plot_memory_usage_by_partition_size(df, "medium", "csv", "creation_partitioned")
    plot_memory_usage_by_scheme(df, "medium", "csv", "creation_partitioned")
    
    # Multi-threaded Chart Creation
    
    os.makedirs(f"{RESULTS_DIR}/figures/creation_multi_threaded", exist_ok=True)

    df_0 = load_results(f"{RESULTS_DIR}/creation/creation_multithreaded_1.json")
    df_1 = load_results(f"{RESULTS_DIR}/creation/creation_multithreaded_2.json")
    df_2 = load_results(f"{RESULTS_DIR}/creation/creation_multithreaded_4.json")
    df_3 = load_results(f"{RESULTS_DIR}/creation/creation_multithreaded_8.json")
    df_4 = load_results(f"{RESULTS_DIR}/creation/creation_multithreaded_16.json")
    df_5 = load_results(f"{RESULTS_DIR}/creation/creation_multithreaded_32.json")

    df = pd.concat([df_0, df_1, df_2, df_3, df_4, df_5], ignore_index=True)
    
    plot_throughput_by_thread_count(df, "large", "csv", "creation_multi_threaded")
    plot_memory_usage_by_thread_count(df, "large", "csv", "creation_multi_threaded")
    
    plot_compression_by_scheme(df_0, "large", "csv", "creation_single_threaded", None)