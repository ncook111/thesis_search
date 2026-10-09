#!/usr/bin/env bash
set -euo pipefail

python3 file_format_converter.py warc json 1.warc.gz 1.json
python3 file_format_converter.py warc json 2.warc.gz 2.json
python3 file_format_converter.py warc json 3.warc.gz 3.json
python3 file_format_converter.py warc json 4.warc.gz 4.json

echo "All conversions done"