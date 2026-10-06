#!/usr/bin/env bash
set -euo pipefail

python3 file_format_converter.py warc json warc/1.warc.gz warc/1.json


echo "All conversions done"