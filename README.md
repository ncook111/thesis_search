# SIMDIndex

SIMDIndex allows for the indexing and search of large amounts of semi-structured data, with support for CSV, JSONL and LOGFMT documents. It supports multi-file inputs, as long as each file shares a common schema. It supports a number of different encoding schemes for docID compression, facilitating in-depth comparisons between them in terms of compression, encoding and decoding throughput.


# Setup

```
git clone https://github.com/ncook111/thesis_search.git
cd thesis_search
```

SIMDIndex supports a number of different SIMD instruction set extensions. If on an ARM mac, DSIMD_TARGET should be set to "neon". If on an x86 system, generally "avx2" should be chosen. The supported DSIMD_TARGET's are "neon", "sse", "avx2" and "avx512"

```
cmake -S . -B build -DSIMD_TARGET=avx2
cmake --build build
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

Optionally, the partition size used when creating the index can be set using --partition \<size in mb>, and max thread utilisation can be set with --threads <# threads>

The datasets contained within the repo are too small to trigger partitioning or multi-threading using default settings. Larger versions of these datasets, as well as copies formatted as JSON and LOGFMT, can be downloaded from an S3 bucket here: <https://392852903727-4zkemjsn.ap-southeast-2.console.aws.amazon.com/s3/buckets/s3-thesis?region=ap-southeast-2&prefix=all_input_data/&showversions=false>. Many of the larger versions of these datasets contain multiple files. This is supported by SIMDIndex, as long as each of the files are formatted using the same CSV, JSON or LOGFMT schema.

Multi-threaded utilisation is based on the number of partitions created from the input files. A dataset size of 1GB, and a partition size of 100MB would result in a maximum of 10 threads being utilised if supported by the hardware.

### Index Search
The dataset indexed above contains review data from Amazon. A search for all documents containing the term 'book', and with a review score of '5.0' can be performed using the following command:
```
./build/simdindex --mode search --index sample_index --query '[*] = "book" & [overall] = "5.0"'
```

If a path to the original data is not provided, the command will return the count of matching documents. In order to return all matching documents --data sample_datasets/data_1 can be included in the above command. Additionally the maxiumum number of threads utilised during search can be specified in the same way as during index creation.

Query commands are comprised of a field_name, enclosed in square brackets, followed by a term. Terms can consist of alphabetical strings and numbers, such as "hello", "25", "192.168.0.1". The type of the term does not need to be defined, as the query engines tokenizer will automatically deduce it. Multiple terms can also be chained together, such as "television book". This will not match the exact phrase as written, but will instead match the union of the two terms. Phrase matching is not supported. Range terms can also be used, denoted by wrapping the term in curly brackets instead of quotation marks. Examples are {1,5}, {0.5,} {,99} which are equivalent to matching values within the ranges 1 to 5 inclusive, &ge; 0.5, and &le; 99

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