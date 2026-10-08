#!/usr/bin/env bash
#
# split_equal.sh — split a file into N files that each have the same number
# of lines. Any leftover lines (total % N) at the end are discarded.
#
# Usage:
#   split_equal.sh <input_file> <num_parts> [output_prefix]
#
# Example:
#   split_equal.sh data.csv 4
#     -> data_part1.csv, data_part2.csv, data_part3.csv, data_part4.csv
#
#   split_equal.sh data.csv 12 out/chunk_
#     -> out/chunk_01.csv ... out/chunk_12.csv

set -euo pipefail

usage() {
  echo "Usage: $(basename "$0") <input_file> <num_parts> [output_prefix]" >&2
  exit 1
}

[[ $# -lt 2 || $# -gt 3 ]] && usage

input=$1
parts=$2

# --- Validate arguments -----------------------------------------------------
if [[ ! -f "$input" || ! -r "$input" ]]; then
  echo "Error: '$input' is not a readable file." >&2
  exit 1
fi

if ! [[ "$parts" =~ ^[1-9][0-9]*$ ]]; then
  echo "Error: <num_parts> must be a positive integer (got '$parts')." >&2
  exit 1
fi

# --- Work out output naming --------------------------------------------------
# Keep the input's extension (if it has one) so data.csv -> data_part1.csv.
base=${input##*/}
if [[ "$base" =~ ^.+\.[^.]+$ ]]; then
  ext=".${base##*.}"
  stem=${input%.*}
else
  ext=""
  stem=$input
fi
prefix=${3:-"${stem}_part"}

# Zero-pad part numbers so the files sort correctly (01..12, 001..100, etc.)
width=${#parts}

# --- Count lines -------------------------------------------------------------
echo "Counting lines in '$input'..." >&2
total=$(wc -l < "$input" | tr -d ' ')
# wc -l counts newlines, so add one if the last line has no trailing newline.
if [[ -s "$input" && -n "$(tail -c 1 "$input")" ]]; then
  total=$(( total + 1 ))
fi

per_file=$(( total / parts ))
remainder=$(( total % parts ))
keep=$(( per_file * parts ))

if (( per_file == 0 )); then
  echo "Error: '$input' has $total line(s), which is fewer than $parts parts." >&2
  exit 1
fi

echo "$total lines -> $parts files of $per_file lines each. Writing..." >&2

# Make sure the output directory exists if the prefix includes one.
outdir=$(dirname "$prefix")
mkdir -p "$outdir"

# split writes its pieces into a hidden temp folder beside the output; each
# piece is then renamed into place (instant, since it's the same disk).
# The temp folder is removed on exit, including if you press Ctrl-C.
tmpdir=$(mktemp -d "$outdir/.split_equal.XXXXXX")
trap 'rm -rf "$tmpdir"' EXIT
trap 'exit 130' INT TERM

# split names pieces xaa, xab, ...; work out how many letters are needed.
suffix_len=1
capacity=26
while (( capacity < parts )); do
  suffix_len=$(( suffix_len + 1 ))
  capacity=$(( capacity * 26 ))
done

# --- Split -------------------------------------------------------------------
# head stops reading before the leftover lines; split does the heavy lifting.
# If pv is installed, show a progress bar while it runs.
if command -v pv >/dev/null 2>&1; then
  size=$(wc -c < "$input" | tr -d ' ')
  head -n "$keep" "$input" | pv -s "$size" | split -l "$per_file" -a "$suffix_len" - "$tmpdir/x"
else
  head -n "$keep" "$input" | split -l "$per_file" -a "$suffix_len" - "$tmpdir/x"
fi

i=0
for piece in "$tmpdir"/x*; do
  i=$(( i + 1 ))
  mv "$piece" "$(printf "%s%0${width}d%s" "$prefix" "$i" "$ext")"
done

echo "Done: wrote $parts files of $per_file lines each." >&2
if (( remainder > 0 )); then
  echo "Discarded $remainder leftover line(s) from the end of the file." >&2
fi