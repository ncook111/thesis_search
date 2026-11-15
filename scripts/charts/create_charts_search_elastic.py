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

QUERY_TYPES = { "simple" : "Simple", "union" : "Union", "intersection" : "Intersection", "combination" : "Combination", "complex_combination" : "Complex Combination" }

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

STATS = ["median"]

def save_chart(out, data, chart_title, legend_title, xlabel, ylabel, zero_line=False, logy=False):
    if data.empty:
        print(f"{out}: no data")
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    if logy:  
        data.plot.bar(ax=ax, rot=0, title=chart_title,
                             xlabel=xlabel, ylabel=ylabel, logy=True)
        ax.yaxis.set_major_formatter(ScalarFormatter())
    else:
        data.plot.bar(ax=ax, rot=0, title=chart_title,
                                     xlabel=xlabel, ylabel=ylabel)
    if legend_title:
        ax.legend(title=legend_title, loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0)

    if zero_line:
        ax.axhline(0, color="black", linewidth=0.8)

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

    df["query_type"] = pd.Categorical(
        parts.str[2].map(QUERY_TYPES),
        categories=list(QUERY_TYPES.values()),
    )

    df["dataset"] = parts.str[3].astype(int) + 1
    
    to_ms = {"s": 1e3, "ms": 1, "us": 1e-3, "ns": 1e-6}
    unit = df["time_unit"] if "time_unit" in df else pd.Series("ms", index=df.index)
    df["real_time"] = df["real_time"] * unit.fillna("ms").map(to_ms)

    df["throughput"] = 1 / (df["real_time"] / 1000)

    if "aggregate_name" in df.columns:
        # already aggregated: keep the aggregate rows as they are
        df = df[df["aggregate_name"].isin(["mean", "median", "stddev", "cv", "p95"])].copy()
        df = df.rename(columns={"aggregate_name": "stat"})
    else:
        # raw repetitions: one row per run, so aggregate per benchmark here
        keys = ["scheme", "file_size", "query_type", "dataset"]
        g = df.groupby(keys, observed=True)[["real_time", "throughput", "peak_delta"]]
        df = pd.concat([g.mean().assign(stat="mean"),
                        g.median().assign(stat="median")]).reset_index()

    df["file_size"] = pd.Categorical(df["file_size"], categories=["small", "medium", "large"], ordered=True)

    all_cols = ["file_size", "scheme", "stat", "real_time", "throughput",
                "index_size", "data_size", "dataset", "query_type", "partition_size", "thread_count", "peak_delta"]
    return df.reindex(columns=all_cols)

def plot_latency_by_query_type(df, file_size, folder_name):
    for stat in STATS:
        title = f"Query Latency ({stat}, all datasets, {file_size})"
        out = f"{RESULTS_DIR}/figures/{folder_name}/{stat}_{file_size}_latency_by_query_type.png"

        data = df[(df["stat"] == stat) & (df["file_size"] == file_size)]

        latency = data.pivot_table(index="query_type", columns="scheme", values="real_time",
                                 aggfunc="mean", observed=True)
        latency.columns = latency.columns.astype(str)

        save_chart(out, latency, title, "Encoding Schemes", "Query Type", "Milliseconds")
        save_table(out, latency, "Scheme", "Query Type")

def plot_throughput_by_query_type(df, file_size, folder_name):
    for stat in STATS:
        title = f"Query Throughput ({stat}, all datasets, {file_size})"
        out = f"{RESULTS_DIR}/figures/{folder_name}/{stat}_{file_size}_throughput_by_query_type.png"

        data = df[(df["stat"] == stat) & (df["file_size"] == file_size)]

        throughput = data.pivot_table(index="query_type", columns="scheme", values="throughput",
                                 aggfunc="mean", observed=True)
        throughput.columns = throughput.columns.astype(str)

        save_chart(out, throughput, title, "Encoding Schemes", "Query Type", "Queries/s")
        save_table(out, throughput, "Scheme", "Query Type")

def plot_memory_usage_by_query_type(df, file_size, folder_name):
    for stat in STATS:
        title = f"{stat.capitalize()} Peak Memory Usage During Search (all datasets, {file_size})"
        out = f"{RESULTS_DIR}/figures/{folder_name}/{stat}_{file_size}_memory_by_query_type.png"

        data = df[(df["stat"] == stat) & (df["file_size"] == file_size)]

        memory = data.pivot_table(index="query_type", columns="scheme", values="peak_delta",
                                 aggfunc="mean", observed=True) / 1000000
        memory.columns = memory.columns.astype(str)

        save_chart(out, memory, title, "Encoding Schemes", "Query Type", "Peak Memory Increase (MB)")
        save_table(out, memory, "Scheme", "Query Type")

if __name__ == "__main__":
    for mode in ["warm", "cold"]:
        df_0 = load_results(f"{RESULTS_DIR}/search_elastic/search_simdindex_{mode}.json")
        df_1 = load_results(f"{RESULTS_DIR}/search_elastic/search_elastic_{mode}.json")
        df = pd.concat([df_0, df_1], ignore_index=True)

        folder_name = f"search_elastic_{mode}"
        os.makedirs(f"{RESULTS_DIR}/figures/{folder_name}", exist_ok=True)

        plot_latency_by_query_type(df, "small", folder_name)
        plot_throughput_by_query_type(df, "small", folder_name)
        plot_memory_usage_by_query_type(df, "small", folder_name)
        plot_latency_by_query_type(df, "medium", folder_name)
        plot_throughput_by_query_type(df, "medium", folder_name)
        plot_memory_usage_by_query_type(df, "medium", folder_name)
        plot_latency_by_query_type(df, "large", folder_name)
        plot_throughput_by_query_type(df, "large", folder_name)
        plot_memory_usage_by_query_type(df, "large", folder_name)