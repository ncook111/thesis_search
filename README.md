# SIMDIndex
SIMDIndex allows for the indexing and search of large amounts of semi-structured data, with support for CSV, JSONL and LOGFMT documents. It supports multi-file inputs, as long as each file shares a common schema. It supports a number of different encoding schemes for docID compression, facilitating in-depth comparisons between them in terms of compression, encoding and decoding throughput.


# Setup
```
git clone https://github.com/ncook111/thesis_search.git
cd thesis_search
```

SIMDIndex supports a number of different SIMD instruction set extensions. The `-DSIMD_TARGET` options available, as well as their requirements are listed below:
  | `-DSIMD_TARGET=` | Architecture | CPU must support |
  |---|---|---|
  | `neon` | arm64 / aarch64 | NEON |
  | `sse` | x86_64 | SSE4.2 |
  | `avx2` | x86_64 | AVX2 and SSE4.2 |
  | `avx512bw` | x86_64 | AVX-512F, AVX-512BW, AVX2 and SSE4.2 |
  | `scalar` | arm64 / aarch64 or x86_64 | — |


On an ARM Mac run:
```
cmake -S . -B build -DSIMD_TARGET=neon
cmake --build build
```

Unless running a high end x86 CPU with support for AVX-512 run:
```
cmake -S . -B build -DSIMD_TARGET=avx2
cmake --build build
```

# Tests
Tests can be run with
```
ctest --test-dir build
```

# Usage and Demo

### Sample Data
The repo contains four sample CSV formatted datasets that can be used for testing:
- **Dataset 1 - Amazon Review Data (2018)** Product reviews and metadata from Amazon. <https://cseweb.ucsd.edu/~jmcauley/datasets/amazon_v2/>
- **Dataset 2 - GH Archive** An hourly archive of public GitHub timeline events. <https://www.gharchive.org/>
- **Dataset 3 - Wikimedia Pageviews** Hourly page-view counts for Wikimedia projects. <https://dumps.wikimedia.org/other/pageviews/readme.html>
- **Dataset 4 - OpenAlex** An open catalog of scholarly works, authors, sources, institutions, and topics, distributed as gzip-compressed JSON Lines snapshots. <https://openalex.org/>

### Index Creation
An index of one of these datasets can be created with the command:
```
./build/simdindex --mode create --encoding s4_bpdm --data sample_datasets/data_1 --index sample_index
```

The `--encoding` values which are supported, and the `-DSIMD_TARGET` required to be used during compilation are summarised below:
| Encodings | Need a build with |
  |---|---|
  | `s4_bpd1`, `s4_bpd2`, `s4_bpd4`, `s4_bpdm` | `neon` or `sse` (or higher) |
  | `s8_bpd4`, `s8_bpdm` | `avx2` (or `avx512bw`) |
  | `s16_bpd4`, `s16_bpdm` | `avx512bw` |
  | `gvb`, `pef`, `fls` | any target |

Optionally, the partition size used when creating the index can be set using --partition \<size in mb>, and max thread utilisation can be set with --threads <# threads>

The datasets contained within the repo are too small to trigger partitioning or multi-threading using default settings. Larger versions of these datasets, as well as copies formatted as JSON and LOGFMT, can be downloaded from an S3 bucket here: <hhttps://s3-thesis.s3.ap-southeast-2.amazonaws.com/compressed/all_input_data.tar.gz>. Many of the larger versions of these datasets contain multiple files. This is supported by SIMDIndex, as long as each of the files are formatted using the same CSV, JSON or LOGFMT schema.

Multi-threaded utilisation is based on the number of partitions created from the input files. A dataset size of 1GB, and a partition size of 100MB would result in a maximum of 10 threads being utilised if supported by the hardware.

### Index Search
The dataset indexed above contains review data from Amazon. A search for all documents containing the term 'book', and with a review score of '5.0' can be performed using the following command:
```
./build/simdindex --mode search --index sample_index --query '[*] = "book" & [overall] = "5.0"'
```

If a path to the original data is not provided, the command will return the count of matching documents. In order to return all matching documents `--data sample_datasets/data_1` can be included in the above command. Additionally the maxiumum number of threads utilised during search can be specified in the same way as during index creation. However, as the input datasets are smaller than the default partition size, the `--threads` option will have no effect.
```
./build/simdindex --mode search --index sample_index --data sample_datasets/data_1 --query '[*] = "book" & [overall] = "5.0"' --threads 4
```

Query commands are comprised of a field_name, enclosed in square brackets, followed by a term. Terms can consist of alphabetical strings and numbers, such as "hello", "25", "192.168.0.1". The type of the term does not need to be defined, as the query engines tokenizer will automatically deduce it. Multiple terms can also be chained together, such as "television book". This will not match the exact phrase as written, but will instead match the union of the two terms. Phrase matching is not supported. Range terms can also be used, denoted by wrapping the term in curly brackets instead of quotation marks. Examples are {1,5}, {0.5,} {,99} which are equivalent to matching values within the ranges 1 to 5 inclusive, &ge; 0.5, and &le; 99

```
./build/simdindex --mode search --index sample_index --query '[*] = "book" & [overall] = {1.0,4.0}'
```

### Index Updating
Indexes can be updated to reflected additional files added to the original source folder, or new documents appended to already indexed source files. New files added must contain identical fields to any existing indexed data. This repo contains sample update files for each of the supplied datasets. Copy them into the dataset folders using the command:
```
cp -r sample_update_data/ sample_datasets/
```
Next, an index update can be triggered using the command:
```
./build/simdindex --mode update --data sample_datasets/data_1 --index sample_index
```

This can be confirmed by running the following search command:
```
./build/simdindex --mode search --index sample_index --data sample_datasets/data_1 --query '[*] = "simdindex update confirmation"'
```

### Index Validation
Each index includes corruption detection. It can be performed using the following command:
```
./build/simdindex --mode verify --index sample_index 
```

# Benchmarks
In order to the run all the benchmarks, the full datasets must be downloaded from the aforementioned S3 bucket: <https://s3-thesis.s3.ap-southeast-2.amazonaws.com/compressed/all_input_data.tar.gz>

The benchmarks directory contain a number of C++ benchmarks utilising Googles Benchmark framework. For ease-of-use, the full benchmark suite was separated into a number of bash scripts, that run a smaller subset of benchmarks, outputting their results into the results directory.

  | Benchmark | Description | Command |
  |---|---|---|
  | SIMDIndex Creation Benchmarks | Benchmarks index creation for all datasets utilising all supported compression schemes | `./scripts/benchmarks/run_creation_benchmarks.sh` |
  | SIMDIndex Search Benchmarks | Benchmarks a number of queries across all datasets and dataset sizes, utilising all supported compression schemes | `./scripts/benchmarks/run_search_benchmarks.sh` |
  | SIMDIndex Encoding Benchmarks | Benchmarks all supported compression and delta encoding schemes at a number of input sizes | `./scripts/benchmarks/run_encoding_benchmarks.sh` |
  | SIMDIndex vs GREP | Compares SIMDIndex utilising its CLI and a decompress-then-grep query pipeline, benchmarking index creation/dataset compression and index querying/dataset decompression then grep | `./scripts/benchmarks/grep_bench` |
  | SIMDIndex vs Elasticsearch | Compares SIMDIndex and Elasticsearch, benchmarking index creation and querying | `./scripts/benchmarks/elastic_bench` |

# Chart and Table Creation
This repo also contains python scripts used to create charts and tables out of the benchmarking data. They require `python3` and `pandas`, `matplotlib` and `warcio` to run.

| Chart script | Benchmark data source |
|---|---|
| `create_charts_encoding_simd.py` | `run_encoding_benchmarks.sh` |
| `create_charts_search_simd.py` | `run_search_benchmarks.sh` |
| `create_charts_creation_simd.py` | `run_creation_benchmarks.sh`|
| `create_charts_creation_grep.py`, `create_charts_search_grep.py` | `grep_bench.sh` |
| `create_charts_creation_elastic.py`, `create_charts_search_elastic.py` | `elastic_bench.sh` |