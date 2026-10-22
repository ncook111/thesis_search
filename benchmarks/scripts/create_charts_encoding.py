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

def create_chart_throughput(df, chart_type, key_title, title_type):
    # Throughput charts
    stats = ["mean", "median", "p90", "p99"]
    for stat in stats:
        sub = df[df["stat"] == stat]
        throughput = sub.pivot(index="size", columns="scheme", values="bytes_per_second") / 1e9 # Convert into GB/s

        fig, ax = plt.subplots(figsize=(8, 5))
        throughput.plot(ax=ax, marker="o", logx=True, title=f"{title_type} Throughput ({stat})", ylabel="GB/s")
        
        ax.legend(title=f"{key_title} Schemes")
        ax.minorticks_off()
        ax.grid(alpha=0.3)
        out = f"../results/figures/encoding/{chart_type}_throughput_{stat}.png"
        fig.tight_layout()
        fig.savefig(out, dpi=150)
        plt.close(fig)
        print(f"wrote {out}")

def create_chart_compression(df, type, key_title):
    # Compression Charts
    median = df[df["stat"] == "median"]
    compression = median.pivot(index="size", columns="scheme", values="compression") * 100

    fig, ax = plt.subplots(figsize=(8, 5))
    compression.plot(ax=ax, marker="o", logx=True, title="Compressed Size", ylabel="% of input")
    
    ax.legend(title=f"{key_title} Schemes")
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    out = f"../results/figures/encoding/{type}_compression.png"
    fig.tight_layout()
    fig.savefig(out, dpi=300)
    plt.close(fig)
    print(f"wrote {out}")
    
def create_chart_bits_int(df, type, key_title):
    # Compression Charts
    median = df[df["stat"] == "median"]
    bits_int = median.pivot(index="size", columns="scheme", values="bits_per_int")

    fig, ax = plt.subplots(figsize=(8, 5))
    bits_int.plot(ax=ax, marker="o", logx=True, title="Bits Per Int", ylabel="bits/int")
    
    ax.legend(title=f"{key_title} Schemes")
    ax.minorticks_off()
    ax.grid(alpha=0.3)
    out = f"../results/figures/encoding/{type}_bits_int.png"
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
    
    create_chart_throughput(df_comp_enc, "compression_encoding", "Compression", "Encoding")
    create_chart_compression(df_comp_enc, "compression_encoding", "Compression")
    create_chart_throughput(df_comp_dec, "compression_decoding", "Compression", "Decoding")
    create_chart_bits_int(df_comp_dec, "compression_decoding", "Compression")
    create_chart_throughput(df_delta_enc, "delta_encoding", "Delta", "Encoding")
    create_chart_throughput(df_delta_dec, "delta_decoding", "Delta", "Decoding")
    
