#!/usr/bin/env python3
"""
Download a Wikimedia "pageview_complete" daily dump and convert it to JSONL.

Each input line looks like:
    en.wikipedia Albert_Einstein 736 desktop 4521 A188B190C175...
    (wiki_code, title, page_id, access_method, daily_total, hourly_counts)

Hourly counts use letters A-X for hours 00-23, each followed by its count.
Hours with zero views are omitted.

Usage:
    # Stream straight from dumps.wikimedia.org (no temp file)
    python pageviews_to_jsonl.py 2026-10-01 -o pv.jsonl

    # Only English Wikipedia, gzip the output
    python pageviews_to_jsonl.py 2026-10-01 --wiki en.wikipedia -o pv.jsonl.gz

    # Bots instead of humans
    python pageviews_to_jsonl.py 2026-10-01 --agent automated -o bots.jsonl

    # Already downloaded the .bz2? Pass the path instead of a date
    python pageviews_to_jsonl.py pageviews-20261001-user.bz2 -o pv.jsonl
"""
import argparse
import bz2
import gzip
import json
import os
import re
import sys
import urllib.request
import ssl, certifi

BASE = "https://dumps.wikimedia.org/other/pageview_complete"
USER_AGENT = "pageviews-to-jsonl/1.0 (replace-with-your-email@example.com)"
HOUR_RE = re.compile(r"([A-X])(\d+)")


def dump_url(date: str, agent: str) -> str:
    y, m, d = date.split("-")
    return f"{BASE}/{y}/{y}-{m}/pageviews-{y}{m}{d}-{agent}.bz2"


def open_source(source: str, agent: str):
    """Return a text stream of decompressed lines from a date, URL or local file."""
    if re.fullmatch(r"\d{4}-\d{2}-\d{2}", source):
        source = dump_url(source, agent)
    if source.startswith("http"):
        print(f"Streaming {source}", file=sys.stderr)
        req = urllib.request.Request(source, headers={"User-Agent": USER_AGENT})
        raw = urllib.request.urlopen(req, context=ssl.create_default_context(cafile=certifi.where()))
    else:
        raw = open(source, "rb")
    return bz2.open(raw, "rt", encoding="utf-8", errors="replace")


def parse_line(line: str):
    line = line.rstrip("\n")
    try:
        wiki, rest = line.split(" ", 1)
        # Split from the right so odd titles can't shift the fixed fields
        title, page_id, access, total, hourly = rest.rsplit(" ", 4)
    except ValueError:
        return None
    hours = {f"{ord(h) - 65:02d}": int(n) for h, n in HOUR_RE.findall(hourly)}
    return {
        "wiki": wiki,
        "title": title,
        "page_id": None if page_id == "null" else int(page_id),
        "access": access,
        "views": int(total),
        "hourly": hours,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source", help="YYYY-MM-DD, a dump URL, or a local .bz2 file")
    ap.add_argument("-o", "--output", default="-",
                    help="output file (.jsonl or .jsonl.gz); default stdout")
    ap.add_argument("--agent", default="user", choices=["user", "automated", "spider"],
                    help="which traffic file to fetch (default: user = humans)")
    ap.add_argument("--wiki", action="append",
                    help="keep only these wiki codes, e.g. en.wikipedia (repeatable)")
    ap.add_argument("--min-views", type=int, default=0,
                    help="drop rows with fewer daily views than this")
    args = ap.parse_args()

    wikis = set(args.wiki) if args.wiki else None
    if args.output == "-":
        out = sys.stdout
    elif args.output.endswith(".gz"):
        out = gzip.open(args.output, "wt", encoding="utf-8")
    else:
        out = open(args.output, "w", encoding="utf-8")

    kept = skipped = 0
    with open_source(args.source, args.agent) as src:
        for i, line in enumerate(src, 1):
            # Cheap prefix check before parsing the whole line
            if wikis and line.split(" ", 1)[0] not in wikis:
                continue
            row = parse_line(line)
            if row is None:
                skipped += 1
                continue
            if row["views"] < args.min_views:
                continue
            out.write(json.dumps(row, ensure_ascii=False) + "\n")
            kept += 1
            if i % 5_000_000 == 0:
                print(f"  {i:,} lines read, {kept:,} written", file=sys.stderr)

    if out is not sys.stdout:
        out.close()
    print(f"Done: {kept:,} rows written, {skipped:,} malformed lines skipped",
          file=sys.stderr)


if __name__ == "__main__":
    main()