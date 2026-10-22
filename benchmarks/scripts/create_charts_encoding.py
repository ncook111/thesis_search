#!/usr/bin/env python3
import json
import os

import matplotlib.pyplot as plt
import pandas as pd


def load_results_encoding_decoding(path):

    with open(path) as f:
        df = pd.json_normalize(json.load(f)["benchmarks"])

    df = df[df["aggregate_name"].isin(["mean", "median", "stddev", "cv", "p90", "p99"])].copy()

    parts = df["run_name"].str.split("/")
    df["scheme"] = parts.str[1].str.split("_", n=1).str[1]
    df["size"] = parts.str[2].astype(int)

    df = df.rename(columns={"aggregate_name": "stat"})
    
    all_cols = ["scheme", "size", "stat", "compression", "bytes_per_second", "bits_per_int"]
    return df.reindex(columns=all_cols)

def plot_throughput(df, chart_type, key_title, title_type):
    title = f"{title_type} Throughput ({stat})"
    out = f"../results/figures/encoding/{chart_type}_throughput_{stat}.png"

    stats = ["mean", "median", "p90", "p99"]
    for stat in stats:
        data = df[df["stat"] == stat]
        throughput = data.pivot(index="size", columns="scheme", values="bytes_per_second") / 1e9 # Convert into GB/s

        fig, ax = plt.subplots(figsize=(8, 5))
        throughput.plot(ax=ax, marker="o", logx=True, title=title, ylabel="GB/s")
        
        ax.legend(title=f"{key_title} Schemes")
        ax.minorticks_off()
        ax.grid(alpha=0.3)
        fig.tight_layout()
        fig.savefig(out, dpi=300)
        plt.close(fig)
        print(f"wrote {out}")

def plot_compression(df, type, key_title):
    title = "Compressed Size"
    out = f"../results/figures/encoding/{type}_compression.png"
    
    data = df[df["stat"] == "median"]
    compression = data.pivot(index="size", columns="scheme", values="compression") * 100

    fig, ax = plt.subplots(figsize=(8, 5))
    compression.plot(ax=ax, marker="o", logx=True, title=title, ylabel="% of input")
    
    ax.legend(title=f"{key_title} Schemes")
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    
def plot_bits_int(df, type, key_title):
    title = "Bits Per Int"
    out = f"../results/figures/encoding/{type}_bits_int.png"
    
    data = df[df["stat"] == "median"]
    bits_int = data.pivot(index="size", columns="scheme", values="bits_per_int")

    fig, ax = plt.subplots(figsize=(8, 5))
    bits_int.plot(ax=ax, marker="o", logx=True, title=title, ylabel="bits/int")
    
    ax.legend(title=f"{key_title} Schemes")
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    
    

if __name__ == "__main__":
    df_comp_enc = load_results_encoding_decoding("../results/encodings/compression_encoding.json")
    df_comp_dec = load_results_encoding_decoding("../results/encodings/compression_decoding.json")
    df_delta_enc = load_results_encoding_decoding("../results/encodings/delta_encoding.json")
    df_delta_dec = load_results_encoding_decoding("../results/encodings/delta_decoding.json")
    
    os.makedirs("../results/figures/encoding", exist_ok=True)
    
    plot_throughput(df_comp_enc, "compression_encoding", "Compression", "Encoding")
    plot_compression(df_comp_enc, "compression_encoding", "Compression")
    plot_throughput(df_comp_dec, "compression_decoding", "Compression", "Decoding")
    plot_bits_int(df_comp_dec, "compression_decoding", "Compression")
    plot_throughput(df_delta_enc, "delta_encoding", "Delta", "Encoding")
    plot_throughput(df_delta_dec, "delta_decoding", "Delta", "Decoding")
    
