#!/usr/bin/env python3
import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

from matplotlib.ticker import FuncFormatter, NullFormatter

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

SCHEMES = {
    "gvb": "GVB", "gvb_delta": "GVB + Delta", "pef": "PEF", "fls": "FLS",
    "s4_bpd4": "S4-BP128-D4", "s4_bpdm": "S4-BP128-DM",
    "s8_bpd4": "S8-BP128-D4", "s8_bpdm": "S8-BP128-DM",
    "s16_bpd4": "S16-BP128-D4", "s16_bpdm": "S16-BP128-DM",
    "delta": "Delta", "s4d4": "S4-D4", "s4dm": "S4-DM",
    "s8d4": "S8-D4", "s8dm": "S8-DM", "s16d4": "S16-D4", "s16dm": "S16-DM",
}

def save_table(out, data):
    # Tabular output
    base = os.path.splitext(out)[0]
    table = data.round(3)
    latex_table = table.T.rename_axis("Scheme").reset_index()
    latex_table.columns.name = None
    latex_table.to_latex(f"{base}.tex", index=False, float_format="%.2f", escape=True)
    print(f"wrote table {out}")

def load_results_encoding_decoding(path):

    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    df = df[df["aggregate_name"].isin(["mean", "median", "stddev", "cv", "p95", "p5"])].copy()

    parts = df["run_name"].str.split("/")
    
    scheme_order = list(SCHEMES) 
    
    df["scheme"] = (
        pd.Categorical(parts.str[1].str.split("_", n=1).str[1], categories=scheme_order, ordered=True)
            .rename_categories(SCHEMES) 
    )
    
    df["size"] = parts.str[2].astype(int)
    

    df = df.rename(columns={"aggregate_name": "stat"})
    
    all_cols = ["scheme", "size", "stat", "compression", "bytes_per_second", "bits_per_int"]
    return df.reindex(columns=all_cols)

def load_results_memory_usage(path):
    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    parts = df["run_name"].str.split("/")
    df["run_type"] = parts.str[1].str.split("_", n=1).str[0]
    
    scheme_order = list(SCHEMES) 
    
    df["scheme"] = (
        pd.Categorical(parts.str[1].str.split("_", n=1).str[1], categories=scheme_order, ordered=True)
            .rename_categories(SCHEMES) 
    )
    
    df["size"] = parts.str[2].astype(int)

    all_cols = ["run_type", "scheme", "size", "peak_delta", "run_name"]
    return df.reindex(columns=all_cols)

def plot_throughput(df, chart_type, key_title, title_type):
    stats = ["median"]
    for stat in stats:
        title = f"{title_type} Throughput ({stat})"
        out = f"{RESULTS_DIR}/figures/encodings_warm/{chart_type}_throughput_{stat}.png"
        data = df[df["stat"] == stat]
        throughput = data.pivot(index="size", columns="scheme", values="bytes_per_second") / 1e9 # Convert into GB/s

        fig, ax = plt.subplots(figsize=(8, 5))
        throughput.plot(ax=ax, marker="o", logx=True, title=title, xlabel="Number of Integers (uint32)", ylabel="GB/s")
        
        ax.legend(title=f"{key_title} Schemes", loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0)
        ax.minorticks_off()
        ax.grid(alpha=0.3)
        fig.tight_layout()
        fig.savefig(out, dpi=300)
        plt.close(fig)
        print(f"wrote {out}")
        
        save_table(out, throughput)

def plot_compression(df, type, key_title):
    title = "Compressed Size"
    out = f"{RESULTS_DIR}/figures/encodings_warm/{type}_compression.png"
    
    data = df[df["stat"] == "median"]
    compression = data.pivot(index="size", columns="scheme", values="compression") * 100

    fig, ax = plt.subplots(figsize=(8, 5))
    compression.plot(ax=ax, marker="o", logx=True, title=title, xlabel="Number of Integers (uint32)", ylabel="% of input")
    
    ax.legend(title=f"{key_title} Schemes", loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0)
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    
    save_table(out, compression)
    
def plot_bits_int(df, type, key_title):
    title = "Bits Per Int"
    out = f"{RESULTS_DIR}/figures/encodings_warm/{type}_bits_int.png"
    
    data = df[df["stat"] == "median"]
    bits_int = data.pivot(index="size", columns="scheme", values="bits_per_int")

    fig, ax = plt.subplots(figsize=(8, 5))
    bits_int.plot(ax=ax, marker="o", logx=True, title=title, xlabel="Number of Integers (uint32)", ylabel="bits/int")
    
    ax.legend(title=f"{key_title} Schemes", loc="upper left", bbox_to_anchor=(1.01, 1), borderaxespad=0)
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    
    save_table(out, bits_int)
    
def plot_memory_usage_by_size(df, type_filter, type_name, key_title, title_type):
    title = f"{title_type} Memory Usage By Scheme and Size"
    out = f"{RESULTS_DIR}/figures/encodings_memory_usage/{type_name}_memory_usage.png"

    data = df[df["run_type"] == type_filter]
    memory = (
        data.groupby(["size", "scheme"], observed=True)["peak_delta"].mean()
            .unstack("scheme") / 1e6          # rows = size, columns = scheme, in MB
    )
    memory.columns = memory.columns.astype(str)

    fig, ax = plt.subplots(figsize=(10, 5))
    memory.plot.bar(ax=ax, rot=0, title=title, xlabel="Input Size", ylabel="Peak memory (MB)", logy=True)
    ax.legend(title=f"{key_title} Scheme", loc="upper left",
              bbox_to_anchor=(1.01, 1), borderaxespad=0)
    
    ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f"{y:g}"))
    ax.yaxis.set_minor_formatter(NullFormatter())

    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {out}")

    save_table(out, memory)
    

if __name__ == "__main__":
    df_comp_enc = load_results_encoding_decoding(f"{RESULTS_DIR}/encodings_warm/compression_encoding.json")
    df_comp_dec = load_results_encoding_decoding(f"{RESULTS_DIR}/encodings_warm/compression_decoding.json")
    df_delta_enc = load_results_encoding_decoding(f"{RESULTS_DIR}/encodings_warm/delta_encoding.json")
    df_delta_dec = load_results_encoding_decoding(f"{RESULTS_DIR}/encodings_warm/delta_decoding.json")
    df_memory_usage = load_results_memory_usage(f"{RESULTS_DIR}/encodings_memory_usage/encodings_memory_usage.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/encodings_warm", exist_ok=True)
    os.makedirs(f"{RESULTS_DIR}/figures/encodings_memory_usage", exist_ok=True)
    
    plot_throughput(df_comp_enc, "compression_encoding", "Compression", "Encoding")
    plot_compression(df_comp_enc, "compression_encoding", "Compression")
    plot_throughput(df_comp_dec, "compression_decoding", "Compression", "Decoding")
    plot_bits_int(df_comp_dec, "compression_decoding", "Compression")
    plot_throughput(df_delta_enc, "delta_encoding", "Delta", "Encoding")
    plot_throughput(df_delta_dec, "delta_decoding", "Delta", "Decoding")
    plot_memory_usage_by_size(df_memory_usage, "cenc", "compression_encoding", "Compression", "Compression Encoding")
    plot_memory_usage_by_size(df_memory_usage, "cdec", "compression_decoding", "Compression", "Compression Decoding")
    plot_memory_usage_by_size(df_memory_usage, "denc", "delta_encoding", "Delta", "Delta Encoding")
    plot_memory_usage_by_size(df_memory_usage, "ddec", "delta_decoding", "Delta", "Delta Decoding")