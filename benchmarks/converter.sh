#!/usr/bin/env bash
set -euo pipefail

python3 file_format_converter.py json csv 5_small_1.json 5_small_1.csv


echo "All conversions done"