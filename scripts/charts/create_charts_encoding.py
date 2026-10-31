#!/usr/bin/env python3
import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = ROOT / "results"

def load_results_encoding_decoding(path):

    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    df = df[df["aggregate_name"].isin(["mean", "median", "stddev", "cv", "p90", "p99", "p10", "p1"])].copy()

    parts = df["run_name"].str.split("/")
    df["scheme"] = parts.str[1].str.split("_", n=1).str[1]
    df["size"] = parts.str[2].astype(int)

    df = df.rename(columns={"aggregate_name": "stat"})
    
    all_cols = ["scheme", "size", "stat", "compression", "bytes_per_second", "bits_per_int"]
    return df.reindex(columns=all_cols)

def load_results_memory_usage(path):
    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    parts = df["run_name"].str.split("/")
    df["run_type"] = parts.str[0]
    df["scheme"] = parts.str[1].str.split("_", n=1).str[1]
    df["size"] = parts.str[2].astype(int)
    
    all_cols = ["scheme", "size", "peak_delta", "run_name"]
    return df.reindex(columns=all_cols)

def plot_throughput(df, chart_type, key_title, title_type):
    stats = ["median", "p10", "p1"]
    for stat in stats:
        title = f"{title_type} Throughput ({stat})"
        out = f"{RESULTS_DIR}/figures/encoding/{chart_type}_throughput_{stat}.png"
        data = df[df["stat"] == stat]
        throughput = data.pivot(index="size", columns="scheme", values="bytes_per_second") / 1e9 # Convert into GB/s

        fig, ax = plt.subplots(figsize=(8, 5))
        throughput.plot(ax=ax, marker="o", logx=True, title=title, xlabel="Number of Integers (uint32)", ylabel="GB/s")
        
        ax.legend(title=f"{key_title} Schemes")
        ax.minorticks_off()
        ax.grid(alpha=0.3)
        fig.tight_layout()
        fig.savefig(out, dpi=300)
        plt.close(fig)
        print(f"wrote {out}")

def plot_compression(df, type, key_title):
    title = "Compressed Size"
    out = f"{RESULTS_DIR}/figures/encoding/{type}_compression.png"
    
    data = df[df["stat"] == "median"]
    compression = data.pivot(index="size", columns="scheme", values="compression") * 100

    fig, ax = plt.subplots(figsize=(8, 5))
    compression.plot(ax=ax, marker="o", logx=True, title=title, xlabel="Number of Integers (uint32)", ylabel="% of input")
    
    ax.legend(title=f"{key_title} Schemes")
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    
def plot_bits_int(df, type, key_title):
    title = "Bits Per Int"
    out = f"{RESULTS_DIR}/figures/encoding/{type}_bits_int.png"
    
    data = df[df["stat"] == "median"]
    bits_int = data.pivot(index="size", columns="scheme", values="bits_per_int")

    fig, ax = plt.subplots(figsize=(8, 5))
    bits_int.plot(ax=ax, marker="o", logx=True, title=title, xlabel="Number of Integers (uint32)", ylabel="bits/int")
    
    ax.legend(title=f"{key_title} Schemes")
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    
def plot_memory_usage_by_scheme(df, type_filter, type_name, key_title, title_type):
    title = f"{title_type} Memory Usage By Scheme"
    pass
    

if __name__ == "__main__":
    df_comp_enc = load_results_encoding_decoding(f"{RESULTS_DIR}/encodings/compression_encoding.json")
    df_comp_dec = load_results_encoding_decoding(f"{RESULTS_DIR}/encodings/compression_decoding.json")
    df_delta_enc = load_results_encoding_decoding(f"{RESULTS_DIR}/encodings/delta_encoding.json")
    df_delta_dec = load_results_encoding_decoding(f"{RESULTS_DIR}/encodings/delta_decoding.json")
    df_memory_usage = load_results_memory_usage(f"{RESULTS_DIR}/encodings/memory_usage.json")
    
    os.makedirs(f"{RESULTS_DIR}/figures/encoding", exist_ok=True)
    
    plot_throughput(df_comp_enc, "compression_encoding", "Compression", "Encoding")
    plot_compression(df_comp_enc, "compression_encoding", "Compression")
    plot_throughput(df_comp_dec, "compression_decoding", "Compression", "Decoding")
    plot_bits_int(df_comp_dec, "compression_decoding", "Compression")
    plot_throughput(df_delta_enc, "delta_encoding", "Delta", "Encoding")
    plot_throughput(df_delta_dec, "delta_decoding", "Delta", "Decoding")
    plot_memory_usage_by_scheme(df_memory_usage, "cenc", "compression_encoding", "Compression", "Compression Encoding")
    plot_memory_usage_by_scheme(df_memory_usage, "cdec", "compression_decoding", "Compression", "Compression Decoding")
    plot_memory_usage_by_scheme(df_memory_usage, "denc", "delta_encoding", "Delta", "Delta Encoding")
    plot_memory_usage_by_scheme(df_memory_usage, "ddec", "delta_decoding", "Delta", "Delta Decoding")
    
