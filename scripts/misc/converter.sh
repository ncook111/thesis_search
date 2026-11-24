#!/usr/bin/env bash
set -euo pipefail

python3 file_format_converter.py csv logfmt csv/medium_data/data_2/2_medium.csv logfmt/medium_data/data_2/2_medium.logfmt
python3 file_format_converter.py csv logfmt csv/medium_data/data_3/3_medium.csv logfmt/medium_data/data_3/3_medium.logfmt
python3 file_format_converter.py csv logfmt csv/medium_data/data_4/4_medium.csv logfmt/medium_data/data_4/4_medium.logfmt

python3 file_format_converter.py csv logfmt csv/small_data/data_1/1_small.csv logfmt/small_data/data_1/1_small.logfmt
python3 file_format_converter.py csv logfmt csv/small_data/data_2/2_small.csv logfmt/small_data/data_2/2_small.logfmt
python3 file_format_converter.py csv logfmt csv/small_data/data_3/3_small.csv logfmt/small_data/data_3/3_small.logfmt
python3 file_format_converter.py csv logfmt csv/small_data/data_4/4_small.csv logfmt/small_data/data_4/4_small.logfmt

echo "All conversions done"