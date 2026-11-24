#!/usr/bin/env python3
import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

SCHEMES = {
    "gvb": "GVB + Delta", "pef": "PEF", "fls": "FLS",
    "s4_bpd4": "S4-BP128-D4", "s4_bpdm": "S4-BP128-DM",
    "s8_bpd4": "S8-BP128-D4", "s8_bpdm": "S8-BP128-DM",
    "s16_bpd4": "S16-BP128-D4", "s16_bpdm": "S16-BP128-DM",
}

QUERY_TYPES = { "simple" : "Simple", "union" : "Union", "intersection" : "Intersection", "combination" : "Combination", "complex_combination" : "Complex Combination" }

FILE_SIZES = { "small" : "Small", "medium" : "Medium", "large" : "Large" }

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

def save(out, data, chart_title, legend_title, xlabel, ylabel, zero_line=False):
    if data.empty:
        print(f"{out}: no data")
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    data.plot.bar(ax=ax, rot=0,
                  xlabel=xlabel, ylabel=ylabel, legend=bool(legend_title))
    if legend_title:
        ax.legend(title=legend_title, loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0, frameon=False)

    if zero_line:
        ax.axhline(0, color="black", linewidth=0.8)

    ax.set_title(chart_title)
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

    if "aggregate_name" in df.columns:
        df = df[df["aggregate_name"].isin(["mean", "median", "stddev", "cv", "p95"])].copy()
    else:
        # no repetitions: one iteration row per benchmark
        df = df.copy()
        df["aggregate_name"] = "single"
    
    parts = df["label"].str.split(",")
    
    scheme_order = list(SCHEMES)
    
    df["scheme"] = (
        pd.Categorical(parts.str[0], categories=scheme_order, ordered=True).rename_categories(SCHEMES)
    )
        
    df["file_size"] = pd.Categorical(parts.str[1], ordered=True).rename_categories(FILE_SIZES)
    
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1
    df["partition_size"] = parts.str[4].astype(int)
    df["thread_count"] = parts.str[5].astype(int)
    df["query_type"] = pd.Categorical(
        df["name"].str.split("/").str[3].map(QUERY_TYPES),
        categories=list(QUERY_TYPES.values()),
    )
    
    to_ms = {"s": 1e3, "ms": 1, "us": 1e-3, "ns": 1e-6}
    unit = df["time_unit"] if "time_unit" in df else pd.Series("ms", index=df.index)
    df["real_time"] = df["real_time"] * unit.fillna("ms").map(to_ms)

    df = df.rename(columns={"aggregate_name": "stat"})

    timed = df["stat"].isin(["mean", "median", "p95"])
    df["throughput"] = df["items_per_second"].where(timed)
    
    df["file_size"] = pd.Categorical(df["file_size"], categories=["Small", "Medium", "Large"], ordered=True)

    all_cols = ["file_type", "file_size", "scheme", "stat", "real_time", "throughput",
                "index_size", "data_size", "dataset", "query_type", "partition_size", "thread_count", "peak_delta"]
    return df.reindex(columns=all_cols)

def plot_latency_by_query_type(df, dataset, file_type, file_size):
    if dataset is None:
        ds_title, ds_file = "all datasets (mean)", "all_datasets"
    else:
        ds_title, ds_file = f"dataset {dataset}", f"dataset_{dataset}"

    stats = ["median"]
    for stat in stats:
        title = f"Query Latency ({stat}, {ds_title}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/{ds_file}_{stat}_{file_size}_{file_type}_latency_by_query.png"
        data = df[df["stat"] == stat].copy()
        if dataset is not None:
            data = data[data["dataset"] == dataset]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        # one dataset: a single value per cell; all datasets: mean across them
        latency = data.pivot_table(index="query_type", columns="scheme", values="real_time",
                                   aggfunc="mean", observed=True)

        save(out, latency, title, "Encoding Schemes", "Query Type", "Milliseconds")
        save_table(out, latency, "Scheme", "Query Type")
        
def plot_latency_by_query_type_and_file_size(df, query_type, folder_name):
    stats = ["median"]
    for stat in stats:
        title = f"Query Latency ({stat}, All datasets, {query_type})"
        out = f"{RESULTS_DIR}/figures/{folder_name}/{stat}_{query_type}_latency_by_query_type_file_size.png"

        data = df[(df["stat"] == stat) & (df["query_type"] == query_type)]

        latency = data.pivot_table(index="file_size", columns="scheme",
                                   values="real_time", aggfunc="mean", observed=True)
        latency.columns = latency.columns.astype(str)
        latency.index = latency.index.astype(str)

        save(out, latency, title, "Encoding Schemes", "Dataset Size", "Milliseconds")
        save_table(out, latency, "Scheme", "Dataset Size")
    
def plot_latency_p95_vs_median_by_query_type(df, dataset, file_type, file_size):
    if dataset is None:
        ds_title, ds_file = "all datasets (mean)", "all_datasets"
    else:
        ds_title, ds_file = f"dataset {dataset}", f"dataset_{dataset}"

    title = f"p95 vs Median Query Latency ({ds_title}, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_single_threaded/{ds_file}_{file_size}_{file_type}_latency_p95_vs_median_by_query.png"

    data = df[df["stat"].isin(["median", "p95"])]
    if dataset is not None:
        data = data[data["dataset"] == dataset]
    data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

    latency = data.pivot(index=["query_type", "dataset", "scheme"], columns="stat", values="real_time")
    if latency.empty or "median" not in latency.columns or "p95" not in latency.columns:
        print(f"{out}: no data")
        return

    # % difference per dataset, then averaged across datasets for each query type
    diff = (latency["p95"] - latency["median"]) / latency["median"] * 100
    diff = diff.groupby(level=["query_type", "scheme"], observed=True).mean().unstack("scheme")

    save(out, diff, title, "Encoding Schemes", "Query Type", "% Higher p95 Latency than Median")
    save_table(out, diff, "Scheme", "Query Type")
    
def plot_latency_p95_vs_median_by_query_type_and_file_size(df, query_type, folder_name):
    title = f"p95 vs Median Query Latency (All datasets, {query_type})"
    out = f"{RESULTS_DIR}/figures/{folder_name}/{query_type}_latency_p95_vs_median_by_file_size.png"

    data = df[df["stat"].isin(["median", "p95"]) & (df["query_type"] == query_type)]

    latency = data.pivot_table(index=["file_size", "file_type", "dataset", "scheme"], columns="stat",
                               values="real_time", aggfunc="mean", observed=True)
    if latency.empty or "median" not in latency.columns or "p95" not in latency.columns:
        print(f"{out}: no data")
        return

    # % difference per dataset/file type, then averaged for each file size
    diff = (latency["p95"] - latency["median"]) / latency["median"] * 100
    diff = diff.groupby(level=["file_size", "scheme"], observed=True).mean().unstack("scheme")
    diff.columns = diff.columns.astype(str)
    diff.index = diff.index.astype(str)

    save(out, diff, title, "Encoding Schemes", "Dataset Size", "% Higher p95 Latency than Median")
    save_table(out, diff, "Scheme", "Dataset Size")
    
def plot_throughput_by_query_type(df, dataset, file_type, file_size):
    if dataset is None:
        ds_title, ds_file = "all datasets", "all_datasets"
    else:
        ds_title, ds_file = f"dataset {dataset}", f"dataset_{dataset}"

    stats = ["median"]
    for stat in stats:
        title = f"Query Throughput ({stat}, {ds_title}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/{ds_file}_{stat}_{file_size}_{file_type}_throughput_by_query.png"
        data = df[df["stat"] == stat].copy()
        if dataset is not None:
            data = data[data["dataset"] == dataset]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        throughput = data.pivot(index=["query_type", "dataset"], columns="scheme", values="throughput")
        if dataset is None:
            throughput = throughput.dropna()   # only cells every scheme has, so the workloads match

        inv = (1.0 / throughput).groupby(level="query_type", observed=True)
        throughput = inv.count() / inv.sum()

        save(out, throughput, title, "Encoding Schemes", "Query Type", "Queries/s")
        save_table(out, throughput, "Scheme", "Query Type")

def plot_throughput_by_threads_schemes(df, query_type, file_type, file_size):
    qt_name = QUERY_TYPES.get(query_type, query_type)
    qt_title = qt_name if query_type is not None else "all queries"
    qt_file = query_type if query_type is not None else "all_queries"

    title = f"Median Query Throughput by Thread Count ({qt_title}, all datasets, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_multi_threaded/{qt_file}_all_datasets_{file_size}_{file_type}_throughput_by_threads_schemes.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if query_type is not None:
        data = data[data["query_type"] == qt_name]
        index = ["dataset", "thread_count"]
    else:
        index = ["dataset", "query_type", "thread_count"]

    throughput = data.pivot(index=index, columns="scheme", values="throughput")
    throughput = throughput.dropna()   # only cells every scheme has, so the workloads match
    if throughput.empty:
        return

    # Each (dataset[, query_type]) cell run once, one after another, per thread count
    inv = (1.0 / throughput).groupby(level="thread_count")
    combined = inv.count() / inv.sum()   # rows = thread counts, cols = schemes

    save(out, combined, title, "Encoding Schemes", "Thread Count", "Queries/s")
    save_table(out, combined, "Scheme", "Thread Count")

    
def plot_throughput_increase_by_query_type(df, dataset, file_type, file_size, baseline="gvb"):
    baseline_name = SCHEMES.get(baseline, baseline)
    if dataset is None:
        ds_title, ds_file = "all datasets", "all_datasets"
    else:
        ds_title, ds_file = f"dataset {dataset}", f"dataset_{dataset}"

    title = f"Extra Throughput vs {baseline_name} (median, {ds_title}, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_single_threaded/{ds_file}_{file_size}_{file_type}_extra_throughput_vs_{baseline}_by_query.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if dataset is not None:
        data = data[data["dataset"] == dataset]

    throughput = data.pivot(index=["query_type", "dataset"], columns="scheme", values="throughput")
    throughput = throughput.dropna()   # only cells every scheme has, so the workloads match
    if throughput.empty or baseline_name not in throughput.columns:
        print(f"{out}: no data")
        return

    # per query type: throughput assuming every dataset runs once, one after another
    inv = (1.0 / throughput).groupby(level="query_type", observed=True)
    combined = inv.count() / inv.sum()

    extra = combined.sub(combined[baseline_name], axis=0).drop(columns=baseline_name)

    save(out, extra, title, "Encoding Schemes", "Query Type", f"Extra Queries/s vs {baseline_name}", True)
    save_table(out, extra, "Scheme", "Query Type")

def plot_latency_decrease_by_query_type(df, dataset, file_type, file_size, baseline="gvb"):
    baseline_name = SCHEMES.get(baseline, baseline)
    if dataset is None:
        ds_title, ds_file = "all datasets (mean)", "all_datasets"
    else:
        ds_title, ds_file = f"dataset {dataset}", f"dataset_{dataset}"

    title = f"Latency Decrease vs {baseline_name} (median, {ds_title}, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_single_threaded/{ds_file}_{file_size}_{file_type}_decrease_vs_{baseline}_by_query.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if dataset is not None:
        data = data[data["dataset"] == dataset]

    latency = data.pivot(index=["query_type", "dataset"], columns="scheme", values="real_time")
    if latency.empty or baseline_name not in latency.columns:
        print(f"{out}: no data")
        return

    # % decrease per dataset, then averaged across datasets for each query type
    base = latency[baseline_name]
    decrease = latency.rsub(base, axis=0).div(base, axis=0) * 100
    decrease = decrease.groupby(level="query_type", observed=True).mean()
    decrease = decrease.drop(columns=baseline_name).dropna(how="all")

    save(out, decrease, title, "Encoding Schemes", "Query Type", f"% Lower Latency than {baseline_name}", True)
    save_table(out, decrease, "Scheme", "Query Type")

def plot_memory_usage_by_query(df, file_type, file_size):
    title = f"Median Peak Memory Usage During Search (all datasets, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_single_threaded/all_datasets_{file_size}_{file_type}_memory_by_query.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]

    memory = data.pivot_table(index="query_type", columns="scheme", values="peak_delta",
                              aggfunc="mean", observed=True) / (1024 ** 2)

    save(out, memory, title, "Encoding Scheme", "Query Type", "Peak Memory Increase (MB)")
    save_table(out, memory, "Scheme", "Query Type")
    
def plot_memory_usage_by_query_type_and_file_size(df, query_type, folder_name):
    stats = ["median"]
    for stat in stats:
        title = f"Peak Memory Usage During Search ({stat}, All datasets, {query_type})"
        out = f"{RESULTS_DIR}/figures/{folder_name}/{stat}_{query_type}_memory_by_query_type_file_size.png"

        data = df[(df["stat"] == stat) & (df["query_type"] == query_type)]

        memory = data.pivot_table(index="file_size", columns="scheme",
                                    values="peak_delta", aggfunc="mean", observed=True) / (1024 ** 2)
        memory.columns = memory.columns.astype(str)
        memory.index = memory.index.astype(str)

        save(out, memory, title, "Encoding Schemes", "Dataset Size", "Peak Memory Increase (MB)")
        save_table(out, memory, "Scheme", "Dataset Size")
    
def plot_memory_usage_by_threads(df, file_type, file_size):
    title = f"Peak Memory Usage During Search (all datasets/queries/schemes, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_multi_threaded/all_{file_size}_{file_type}_memory_by_threads.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)]

    # Arithmetic mean across datasets, query types and schemes, bytes -> MiB
    memory = (data.groupby("thread_count")["peak_delta"].mean() / (1024 ** 2)).to_frame("peak")

    save(out, memory, title, None, "Thread Count", "Peak Memory Increase (MiB)")
    save_table(out, memory["peak"], "Thread Count", "Peak Memory Increase (MiB)")

if __name__ == "__main__":
    df_cold = load_results(f"{RESULTS_DIR}/search/search_cold.json")
    df_warm = load_results(f"{RESULTS_DIR}/search/search_warm.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/search_single_threaded_cold", exist_ok=True)
    os.makedirs(f"{RESULTS_DIR}/figures/search_single_threaded_warm", exist_ok=True)
    
    for df, folder_path in [(df_warm, "search_single_threaded_warm"), (df_cold, "search_single_threaded_cold")]:
        file_sizes = df["file_size"].unique()
        file_types = df["file_type"].unique()
        query_types = ["Simple", "Union", "Intersection", "Combination", "Complex Combination"]
        for query_type in query_types:
            plot_latency_by_query_type_and_file_size(df, query_type, folder_path)
            plot_memory_usage_by_query_type_and_file_size(df, query_type, folder_path)
            plot_latency_p95_vs_median_by_query_type_and_file_size(df, query_type, folder_path)   
    
    # Multithreading
    df_1 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_1.json")
    df_2 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_2.json")
    df_3 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_4.json")
    df_4 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_8.json")
    df_5 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_16.json")
    df_6 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_32.json")
    df_mem = load_results(f"{RESULTS_DIR}/search/search_large_memory_usage_threads.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/search_multi_threaded", exist_ok=True)
    
    query_types = [None, "simple", "union", "intersection", "combination", "complex_combination"]
    df = pd.concat([df_1, df_2, df_3, df_4, df_5, df_6], ignore_index=True)
    for query in query_types:
        for file_type in file_types:
            for file_size in file_sizes:
                plot_throughput_by_threads_schemes(df, query, file_type, file_size)
                plot_memory_usage_by_threads(df_mem, file_type, file_size)