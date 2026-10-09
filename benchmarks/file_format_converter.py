import re
import sys
import csv
import json
from warcio.archiveiterator import ArchiveIterator


csv.field_size_limit(sys.maxsize)
BUF = 1 << 20  # 1 MB
ENC = 'utf-8'
CHARSET_RE = re.compile(r'charset=["\']?([\w.-]+)', re.I)
TEXT_TYPES = ('text/', 'application/xhtml', 'application/json', 'application/xml')
WARC_FIELDS = ['url', 'date', 'status', 'content_type', 'body']
WAT_FIELDS = ['url', 'date', 'record_type', 'status', 'content_type', 'title', 'links']


def csv_to_json(in_path, out_path):
    with open(in_path, 'r', newline='', encoding=ENC, buffering=BUF) as csv_file, \
         open(out_path, 'w', encoding=ENC, buffering=BUF) as out_file:
        reader = csv.DictReader(csv_file)
        out_file.writelines(json.dumps(row, ensure_ascii=False) + '\n' for row in reader)

def csv_to_logfmt(in_path, out_path):
    with open(in_path, 'r', newline='', encoding=ENC, buffering=BUF) as csv_file, \
         open(out_path, 'w', encoding=ENC, buffering=BUF) as log_file:
        reader = csv.DictReader(csv_file)
        log_file.writelines(to_logfmt(row) for row in reader)

def json_to_csv(in_path, out_path):
    # Collect all keys first
    fieldnames = {}
    with open(in_path, 'r', encoding=ENC, buffering=BUF) as in_file:
        for line in in_file:
            if line.strip():
                fieldnames.update(dict.fromkeys(json.loads(line)))

    # Stream rows to output file
    with open(in_path, 'r', encoding=ENC, buffering=BUF) as in_file, \
         open(out_path, 'w', newline='', encoding=ENC, buffering=BUF) as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=list(fieldnames))
        writer.writeheader()
        writer.writerows(json.loads(line) for line in in_file if line.strip())

def json_to_logfmt(in_path, out_path):
    with open(in_path, 'r', encoding=ENC, buffering=BUF) as in_file, \
         open(out_path, 'w', encoding=ENC, buffering=BUF) as log_file:
        log_file.writelines(to_logfmt(json.loads(line)) for line in in_file if line.strip())

def warc_to_csv(in_path, out_path):
    with open(out_path, 'w', newline='', encoding=ENC, buffering=BUF) as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=WARC_FIELDS)
        writer.writeheader()
        writer.writerows(iter_warc_pages(in_path))

def warc_to_json(in_path, out_path):
    with open(out_path, 'w', encoding=ENC, buffering=BUF) as out_file:
        out_file.writelines(json.dumps(p, ensure_ascii=False) + '\n' for p in iter_warc_pages(in_path))

def warc_to_logfmt(in_path, out_path):
    with open(out_path, 'w', encoding=ENC, buffering=BUF) as log_file:
        log_file.writelines(to_logfmt(p) for p in iter_warc_pages(in_path))

def wat_to_csv(in_path, out_path):
    with open(out_path, 'w', newline='', encoding=ENC, buffering=BUF) as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=WAT_FIELDS)
        writer.writeheader()
        writer.writerows(flatten_wat(r) for r in iter_wat_records(in_path))

def wat_to_json(in_path, out_path):
    # Full WAT JSON object per line, exactly as stored in the file
    with open(out_path, 'w', encoding=ENC, buffering=BUF) as out_file:
        out_file.writelines(json.dumps(r, ensure_ascii=False) + '\n' for r in iter_wat_records(in_path))

def wat_to_logfmt(in_path, out_path):
    with open(out_path, 'w', encoding=ENC, buffering=BUF) as log_file:
        log_file.writelines(to_logfmt(flatten_wat(r)) for r in iter_wat_records(in_path))

def xml_to_csv(in_path, out_path):
    raise NotImplementedError("xml -> csv")

def xml_to_json(in_path, out_path):
    raise NotImplementedError("xml -> json")

def xml_to_logfmt(in_path, out_path):
    raise NotImplementedError("xml -> logfmt")

def txt_to_csv(in_path, out_path):
    raise NotImplementedError("txt -> csv")

def txt_to_json(in_path, out_path):
    raise NotImplementedError("txt -> json")

def txt_to_logfmt(in_path, out_path):
    raise NotImplementedError("txt -> logfmt")

def to_logfmt(row):
    return ' '.join(f'{k}={logfmt_value(v)}' for k, v in row.items()) + '\n'

def logfmt_value(v):
    if v is None:
        return ''
    if isinstance(v, bool):
        return 'true' if v else 'false'
    if isinstance(v, (dict, list)):
        v = json.dumps(v, ensure_ascii=False)
    s = str(v)
    if s == '' or any(c in s for c in ' ="\\\n\r\t'):
        s = (s.replace('\\', '\\\\').replace('"', '\\"')
              .replace('\n', '\\n').replace('\r', '\\r').replace('\t', '\\t'))
        return f'"{s}"'
    return s

def iter_warc_pages(in_path):
    with open(in_path, 'rb') as in_file:
        for record in ArchiveIterator(in_file):
            if record.rec_type != 'response' or record.http_headers is None:
                continue

            content_type = record.http_headers.get_header('Content-Type') or ''
            if not content_type.lower().startswith(TEXT_TYPES):
                continue

            raw = record.content_stream().read()
            m = CHARSET_RE.search(content_type)
            charset = m.group(1) if m else 'utf-8'
            try:
                body = raw.decode(charset, errors='replace')
            except LookupError:
                body = raw.decode('utf-8', errors='replace')

            status = record.http_headers.get_statuscode()
            yield {
                'url': record.rec_headers.get_header('WARC-Target-URI'),
                'date': record.rec_headers.get_header('WARC-Date'),
                'status': int(status) if status and status.isdigit() else status,
                'content_type': content_type,
                'body': body,
            }

def iter_wat_records(in_path):
    with open(in_path, 'rb') as in_file:
        for record in ArchiveIterator(in_file):
            if record.rec_type != 'metadata':
                continue
            try:
                yield json.loads(record.content_stream().read())
            except (json.JSONDecodeError, UnicodeDecodeError):
                continue

def flatten_wat(obj):
    # Pull the most useful fields out of the nested WAT structure (for CSV/logfmt)
    env = obj.get('Envelope', {})
    header = env.get('WARC-Header-Metadata', {})
    http = env.get('Payload-Metadata', {}).get('HTTP-Response-Metadata', {})
    html = http.get('HTML-Metadata', {})
    return {
        'url': header.get('WARC-Target-URI'),
        'date': header.get('WARC-Date'),
        'record_type': header.get('WARC-Type'),
        'status': http.get('Response-Message', {}).get('Status'),
        'content_type': http.get('Headers', {}).get('Content-Type'),
        'title': html.get('Head', {}).get('Title'),
        'links': json.dumps(html.get('Links', []), ensure_ascii=False),
    }

CONVERSIONS = {
    ("csv", "json"): csv_to_json,
    ("csv", "logfmt"): csv_to_logfmt,
    ("json", "csv"): json_to_csv,
    ("json", "logfmt"): json_to_logfmt,
    ("warc", "csv"): warc_to_csv,
    ("warc", "json"): warc_to_json,
    ("warc", "logfmt"): warc_to_logfmt,
    ("wat", "csv"): wat_to_csv,
    ("wat", "json"): wat_to_json,
    ("wat", "logfmt"): wat_to_logfmt,
    ("xml", "csv"): xml_to_csv,
    ("xml", "json"): xml_to_json,
    ("xml", "logfmt"): xml_to_logfmt,
    ("txt", "csv"): txt_to_csv,
    ("txt", "json"): txt_to_json,
    ("txt", "logfmt"): txt_to_logfmt,
}

def main():
    if len(sys.argv) != 5:
        sys.exit(f"Usage: {sys.argv[0]} <input file type> <output file type> <path/to/input/file> <path/to/output/file>")

    in_type, out_type, in_path, out_path = sys.argv[1:5]

    if in_type == out_type:
        sys.exit("Cannot convert to same file type")

    converter = CONVERSIONS.get((in_type, out_type))
    if converter is None:
        supported = ", ".join(f"{a}->{b}" for a, b in CONVERSIONS)
        sys.exit(f"Conversion not supported: {in_type}->{out_type} (supported: {supported})")

    converter(in_path, out_path)

if __name__ == "__main__":
    sys.exit(main())