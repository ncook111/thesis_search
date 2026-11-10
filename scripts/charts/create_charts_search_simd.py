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

def save_table(out, data):
    # Tabular output
    base = os.path.splitext(out)[0]
    table = data.round(3)
    latex_table = table.T.rename_axis("Scheme").reset_index()
    latex_table.columns.name = None
    latex_table.to_latex(f"{base}.tex", index=False, float_format="%.2f", escape=True)
    print(f"wrote table {out}")
    
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
    df["scheme"] = pd.Categorical(parts.str[0], categories=["gvb", "pef", "fls", "s4_bpd4", "s4_bpdm", "s8_bpd4", "s8_bpdm", "s16_bpd4", "s16_bpdm"], ordered=True)
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1
    df["partition_size"] = parts.str[4].astype(int)
    df["thread_count"] = parts.str[5].astype(int)
    df["query_type"] = df["name"].str.split("/").str[3]
    
    df = df[~df["scheme"].isin(EXCLUDED_SCHEMES)]

    df = df.rename(columns={"aggregate_name": "stat"})

    timed = df["stat"].isin(["mean", "median", "p95"])
    df["throughput"] = df["items_per_second"].where(timed)

    all_cols = ["file_type", "file_size", "scheme", "stat", "real_time", "throughput",
                "index_size", "data_size", "dataset", "query_type", "partition_size", "thread_count", "peak_delta"]
    return df.reindex(columns=all_cols)

def load_results_grep(path):
    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    if "aggregate_name" in df.columns:
        df = df[df["aggregate_name"].isin(["median"])].copy()
    else:
        # no repetitions: one iteration row per benchmark
        df = df.copy()
        df["aggregate_name"] = "single"
    
    parts = df["label"].str.split(",")
    df["scheme"] = pd.Categorical(parts.str[0], categories=["gvb", "pef", "fls", "s4_bpd4", "s4_bpdm", "s8_bpd4", "s8_bpdm", "s16_bpd4", "s16_bpdm"], ordered=True)
    df["file_size"] = pd.Categorical(parts.str[1], categories=["small", "medium", "large"], ordered=True)
    df["file_type"] = parts.str[2]
    df["dataset"] = parts.str[3].astype(int) + 1
    df["partition_size"] = parts.str[4].astype(int)
    df["thread_count"] = parts.str[5].astype(int)
    df["query_type"] = df["name"].str.split("/").str[3]
    
    df = df[~df["scheme"].isin(EXCLUDED_SCHEMES)]

    df = df.rename(columns={"aggregate_name": "stat"})

    timed = df["stat"].isin(["mean", "median", "p95"])
    df["throughput"] = df["items_per_second"].where(timed)

    all_cols = ["file_type", "file_size", "scheme", "stat", "real_time", "throughput",
                "index_size", "data_size", "dataset", "query_type", "partition_size", "thread_count", "peak_delta"]
    return df.reindex(columns=all_cols)

def plot_latency_by_dataset(df, query_type, file_type, file_size):
    stats = ["median", "p95"]
    for stat in stats:
        title = f"Query Latency ({stat}, {query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/{query_type}_{stat}_{file_size}_{file_type}_latency_by_dataset.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["query_type"] == query_type]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        latency = data.pivot(index="dataset", columns="scheme", values="real_time")

        cols = [s for s in SCHEMES if s in latency.columns]
        cols += [c for c in latency.columns if c not in cols]
        latency = latency[cols].rename(columns=SCHEMES)

        save(out, latency, title, "Encoding Schemes", "Datasets", "Microseconds")

def plot_latency_by_query_type(df, dataset, file_type, file_size):
    stats = ["median", "p95"]
    for stat in stats:
        title = f"Query Latency ({stat}, {dataset}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/dataset_{dataset}_{stat}_{file_size}_{file_type}_latency_by_query.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["dataset"] == dataset] 
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        latency = data.pivot(index="query_type", columns="scheme", values="real_time")

        cols = [s for s in SCHEMES if s in latency.columns]
        cols += [c for c in latency.columns if c not in cols]
        latency = latency[cols].rename(columns=SCHEMES)

        save(out, latency, title, "Encoding Schemes", "Query Type", "Microseconds")

def plot_latency_by_scheme(df, query_type, dataset, file_type, file_size):
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
    
def plot_throughput_by_dataset(df, query_type, file_type, file_size):
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
    
def plot_throughput_by_query_type(df, dataset, file_type, file_size):
    stats = ["median"]
    for stat in stats:
        title = f"Query Throughput ({stat}, dataset {dataset}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/dataset_{dataset}_{stat}_{file_size}_{file_type}_throughput_by_query.png"
        data = df[df["stat"] == stat].copy()
        data = data[data["dataset"] == dataset]
        data = data[(data["file_size"] == file_size) & (data["file_type"] == file_type)]

        throughput = data.pivot(index="query_type", columns="scheme", values="throughput")

        cols = [s for s in SCHEMES if s in throughput.columns]
        cols += [c for c in throughput.columns if c not in cols]
        throughput = throughput[cols].rename(columns=SCHEMES)

        save(out, throughput, title, "Encoding Schemes", "Queries/s")

def plot_throughput_by_scheme_threads(df, query_type, file_type, file_size):
    qt_title = query_type if query_type is not None else "all queries"
    qt_file = query_type if query_type is not None else "all_queries"

    title = f"Median Query Throughput by Thread Count ({qt_title}, all datasets, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_single_threaded/{qt_file}_all_datasets_{file_size}_{file_type}_throughput_by_scheme_threads.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if query_type is not None:
        data = data[data["query_type"] == query_type]
        index = ["dataset", "thread_count"]
    else:
        index = ["dataset", "query_type", "thread_count"]

    throughput = data.pivot(index=index, columns="scheme", values="throughput")
    throughput = throughput.dropna()   # only cells every scheme has, so the workloads match
    if throughput.empty:
        return

    inv = (1.0 / throughput).groupby(level="thread_count")
    combined = (inv.count() / inv.sum()).T

    order = [s for s in SCHEMES if s in combined.index]
    order += [s for s in combined.index if s not in order]
    combined = combined.loc[order].rename(index=SCHEMES)

    save(out, combined, title, "Threads", "Encoding Scheme", "Queries/s")

def plot_throughput_by_threads_schemes(df, query_type, file_type, file_size):
    qt_title = query_type if query_type is not None else "all queries"
    qt_file = query_type if query_type is not None else "all_queries"

    title = f"Median Query Throughput by Thread Count ({qt_title}, all datasets, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_multithreaded/{qt_file}_all_datasets_{file_size}_{file_type}_throughput_by_threads_schemes.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if query_type is not None:
        data = data[data["query_type"] == query_type]
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

    cols = [s for s in SCHEMES if s in combined.columns]
    cols += [c for c in combined.columns if c not in cols]
    combined = combined[cols].rename(columns=SCHEMES)

    save(out, combined, title, "Encoding Schemes", "Thread Count", "Queries/s")
    
def plot_throughput_increase_by_scheme(df, query_type, file_type, file_size, baseline="gvb"):
    baseline_name = SCHEMES.get(baseline, baseline)
    if query_type is not None:
        title = f"Median Extra Throughput vs {baseline_name} ({query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/{query_type}_{file_size}_{file_type}_extra_throughput_vs_{baseline}.png"
    else:
        title = f"Median Extra Throughput vs {baseline_name} ({file_size}, {file_type}, all queries)"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/all_queries_{file_size}_{file_type}_extra_throughput_vs_{baseline}.png"

    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if query_type is not None:
        data = data[data["query_type"] == query_type]
        throughput = data.pivot(index="dataset", columns="scheme", values="throughput")
    else:
        throughput = data.pivot(index=["dataset", "query_type"], columns="scheme", values="throughput")

    throughput = throughput.dropna()   # only cells every scheme has, so the workloads match
    if throughput.empty or baseline not in throughput.columns:
        return

    # throughput assuming every dataset-query combination runs once, one after another
    combined = len(throughput) / (1.0 / throughput).sum()

    extra = (combined - combined[baseline]).drop(baseline).to_frame("median")

    order = [s for s in SCHEMES if s in extra.index]
    order += [s for s in extra.index if s not in order]
    extra = extra.loc[order].rename(index=SCHEMES)

    save(out, extra, title, None, "Encoding Schemes", f"Extra Queries/s vs {baseline_name}", True)

def plot_latency_decrease_by_scheme(df, query_type, file_type, file_size, baseline="gvb"):
    baseline_name = SCHEMES.get(baseline, baseline)
    if query_type is not None:
        title = f"Median Latency Decrease vs {baseline_name} ({query_type}, {file_size}, {file_type})"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/{query_type}_{file_size}_{file_type}_decrease_vs_{baseline}.png"
    else:
        title = f"Median Latency Decrease vs {baseline_name} ({file_size}, {file_type}, all queries)"
        out = f"{RESULTS_DIR}/figures/search_single_threaded/all_queries_{file_size}_{file_type}_decrease_vs_{baseline}.png"

    results = {}
    data = df[(df["stat"] == "median") & (df["file_size"] == file_size) & (df["file_type"] == file_type)]
    if query_type is not None:
        data = data[data["query_type"] == query_type]
        latency = data.pivot(index="dataset", columns="scheme", values="real_time")
    else:
        latency = data.pivot(index=["dataset", "query_type"], columns="scheme", values="real_time")

    if latency.empty or baseline not in latency.columns:
        return

    base = latency[baseline]
    decrease = latency.rsub(base, axis=0).div(base, axis=0) * 100
    results["mediuan"] = decrease.mean().drop(baseline)

    avg = pd.DataFrame(results)   # rows = schemes, columns = statistics

    order = [s for s in SCHEMES if s in avg.index]
    order += [s for s in avg.index if s not in order]
    avg = avg.loc[order].rename(index=SCHEMES)
    
    save(out, avg, title, None, "Encoding Scheme", f"% Lower Latency than {baseline_name}", True)

def plot_memory_usage_by_query(df, file_type, file_size):
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
    
def plot_memory_usage_by_threads(df, file_type, file_size):
    title = f"Peak Memory Usage During Search (all datasets/queries/schemes, {file_size}, {file_type})"
    out = f"{RESULTS_DIR}/figures/search_multithreaded/all_{file_size}_{file_type}_memory_by_threads.png"

    data = df[(df["file_size"] == file_size) & (df["file_type"] == file_type)]

    # Arithmetic mean across datasets, query types and schemes, bytes -> MiB
    memory = (data.groupby("thread_count")["peak_delta"].mean() / (1024 ** 2)).to_frame("peak")

    save(out, memory, title, None, "Thread Count", "Peak Memory Increase (MiB)")

if __name__ == "__main__":
    df = load_results(f"{RESULTS_DIR}/search/search_multithreaded_1.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/search_single_threaded", exist_ok=True)
    
    file_sizes = df["file_size"].unique()
    file_types = df["file_type"].unique()
    datasets = df["dataset"].unique()
    query_types = [None, "simple", "union", "intersection", "combination", "complex_combination"]

    for query in query_types:
        for file_type in file_types:
            for file_size in file_sizes:
                plot_latency_decrease_by_scheme(df, query, file_type, file_size)
                plot_throughput_increase_by_scheme(df, query, file_type, file_size)
                 
    plot_memory_usage_by_query(df, file_type, file_size) 
    
    # Multithreading
    df_1 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_1.json")
    df_2 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_2.json")
    df_3 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_4.json")
    df_4 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_8.json")
    df_5 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_16.json")
    df_6 = load_results(f"{RESULTS_DIR}/search/search_multithreaded_32.json")
    df_mem = load_results(f"{RESULTS_DIR}/search/search_large_memory_usage_threads.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/search_multithreaded", exist_ok=True)
    
    query_types = [None, "simple", "union", "intersection", "combination", "complex_combination"]
    df = pd.concat([df_1, df_2, df_3, df_4, df_5, df_6], ignore_index=True)
    for query in query_types:
        for file_type in file_types:
            for file_size in file_sizes:
                plot_throughput_by_threads_schemes(df, query, file_type, file_size)
                plot_memory_usage_by_threads(df_mem, file_type, file_size)