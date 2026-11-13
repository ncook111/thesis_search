#!/usr/bin/env bash
set -euo pipefail

python3 file_format_converter.py csv json csv/large_data/data_4/4_large_1.csv json/large_data/data_4/4_large_1.json
python3 file_format_converter.py csv json csv/large_data/data_4/4_large_2.csv json/large_data/data_4/4_large_2.json
python3 file_format_converter.py csv json csv/large_data/data_4/4_large_3.csv json/large_data/data_4/4_large_3.json

echo "All conversions done"