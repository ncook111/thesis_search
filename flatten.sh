#!/usr/bin/env bash
set -euo pipefail

python3 flatten_json.py 1.json 1_flat.json
python3 flatten_json.py 2.json 2_flat.json
python3 flatten_json.py 3.json 3_flat.json


echo "All conversions done"