#!/usr/bin/env bash
set -euo pipefail

python3 file_format_converter.py warc json data/1.warc.gz data/1.json
python3 file_format_converter.py warc json data/2.warc.gz data/2.json
python3 file_format_converter.py warc json data/3.warc.gz data/3.json
python3 file_format_converter.py warc json data/4.warc.gz data/4.json


echo "All conversions done"