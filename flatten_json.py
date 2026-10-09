#!/usr/bin/env python3
# Usage: python3 flatten_json.py input.jsonl output.jsonl
#
# Flattens each JSON line to a depth of 1: any key whose value is a JSON
# object is removed, and that object's contents are moved up a level.
# If the same key name appears more than once, the first value found is kept.
import json
import sys


def flatten(obj, out):
    for key, value in obj.items():
        if isinstance(value, dict):
            flatten(value, out)
        elif key not in out:
            out[key] = value
    return out


in_path, out_path = sys.argv[1], sys.argv[2]

with open(in_path, encoding="utf-8") as src, open(out_path, "w", encoding="utf-8") as dst:
    for line in src:
        if line.strip():
            dst.write(json.dumps(flatten(json.loads(line), {}), ensure_ascii=False) + "\n")