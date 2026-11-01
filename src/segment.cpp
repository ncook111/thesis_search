#include <cstdint>
#include <fstream>
#include <cstring>
#include <filesystem>
#include "segment.h"
#include "helper.h"
#include "simdjson.h"
#include "csv.hpp"
#include "logfmt_parser.h"

Offsets_header read_offsets_header(const std::string &file_name) {
  uint64_t offsets_header_pos = get_offsets_header_pos(file_name);
  Offsets_header offsets_header = {};

  std::ifstream in(file_name, std::ios::binary);
  in.seekg(offsets_header_pos);
  in.read(reinterpret_cast<char*>(&offsets_header), sizeof(offsets_header));
  in.close();

  return offsets_header;
}

Offsets_header read_offsets_header(const uint8_t *file) {
  Offsets_header header = {};
  std::memcpy(&header, file + get_offsets_header_pos(file), sizeof(header));
  return header;
}

Segment_header get_segment_header(const std::string &file_name) {
  // Read in header
  std::ifstream in(file_name, std::ios::binary);
  Segment_header segment_header = {};
  in.read(reinterpret_cast<char*>(&segment_header), sizeof(segment_header));
  in.close();

  return segment_header;
}

Segment_header get_segment_header(const uint8_t *file) {
  // Read in header
  Segment_header segment_header = {};
  std::memcpy(&segment_header, file, sizeof(segment_header));

  return segment_header;
}

uint64_t get_offsets_header_pos(const std::string &file_name) {
  Segment_header segment_header = get_segment_header(file_name);

  return segment_header.offsets_header_pos;
}

uint64_t get_offsets_header_pos(const uint8_t *file) {
  Segment_header segment_header = get_segment_header(file);

  return segment_header.offsets_header_pos;
}

uint64_t get_alpha_fst_pos(const std::string &file_name) {
  Segment_header segment_header = get_segment_header(file_name);

  return segment_header.alpha_fst_pos;
}

uint64_t get_alpha_fst_pos(const uint8_t *file) {
  Segment_header segment_header = get_segment_header(file);

  return segment_header.alpha_fst_pos;
}

uint64_t get_float_fst_pos(const std::string &file_name) {
  Segment_header segment_header = get_segment_header(file_name);

  return segment_header.float_fst_pos;
}

uint64_t get_float_fst_pos(const uint8_t *file) {
  Segment_header segment_header = get_segment_header(file);

  return segment_header.float_fst_pos;
}

uint64_t get_fields_fst_pos(const std::string &file_name) {
  Segment_header segment_header = get_segment_header(file_name);

  return segment_header.fields_fst_pos;
}

uint64_t get_fields_fst_pos(const uint8_t *file) {
  Segment_header segment_header = get_segment_header(file);

  return segment_header.fields_fst_pos;
}

void update_offsets_header_pos(std::ofstream &file, const uint64_t value) {
  const std::streampos current_pos = file.tellp();
  file.seekp(0);
  file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  file.seekp(current_pos);
}

void update_alpha_fst_pos(std::ofstream &file, const uint64_t value) {
  const std::streampos current_pos = file.tellp();
  file.seekp(sizeof(uint64_t) * 1);
  file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  file.seekp(current_pos);
}

void update_float_fst_pos(std::ofstream &file, const uint64_t value) {
  const std::streampos current_pos = file.tellp();
  file.seekp(sizeof(uint64_t) * 2);
  file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  file.seekp(current_pos);
}

void update_fields_fst_pos(std::ofstream &file, const uint64_t value) {
  const std::streampos current_pos = file.tellp();
  file.seekp(sizeof(uint64_t) * 3);
  file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  file.seekp(current_pos);
}

void partition_file(const std::string &file_path, const uint64_t file_offset, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifers) {
  uint8_t type = str_to_file_type(file_path);
  switch (type) {
    case FileType::CSV:
      return partition_file_csv(file_path, file_offset, segment_size, segment_offsets, segment_docid_modifers);
    case FileType::JSON:
      return partition_file_json(file_path, file_offset, segment_size, segment_offsets, segment_docid_modifers);
    case FileType::LOGFMT:
      return partition_file_logfmt(file_path, file_offset, segment_size, segment_offsets, segment_docid_modifers);
    default:
      throw std::invalid_argument("File type not supported");
  }
}

void partition_file_csv(const std::string &file_path, const uint64_t file_offset, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers) {
  // Open at front of file to get column names
  std::vector<std::string> col_names;
  {
    csv::CSVFormat format;
    format.variable_columns(csv::VariableColumnPolicy::THROW);
    col_names = csv::get_col_names(file_path, format);
  }

  // Reopen starting from file_offset
  auto in = std::make_unique<std::ifstream>(file_path, std::ios::binary);
  if (!*in) throw std::runtime_error("cannot open " + file_path);
  in->seekg(static_cast<std::streamoff>(file_offset));
  if (!*in) throw std::runtime_error("seek failed");

  csv::CSVFormat format;
  format.column_names(col_names)
        .delimiter(',')
        .variable_columns(csv::VariableColumnPolicy::THROW);

  // Header already read, so disregard it if reading from start of file
  if (file_offset == 0)
    format.header_row(0);

  csv::CSVReader reader(std::move(in), format);

  bool first = true;
  uint64_t segment_start = 0;
  uint64_t num_documents = 0;
  segment_docid_modifiers.push_back(num_documents);
  for (const csv::CSVRow &row : reader) { // Input iterator
    // byte_offset() is relative to the stream start (file_offset)
    const uint64_t pos = file_offset + row.byte_offset();

    if (first) { 
      segment_start = pos; 
      first = false; 
    } else if (pos - segment_start > segment_size) {
      segment_offsets.push_back(segment_start);
      segment_start = pos;
      segment_docid_modifiers.push_back(num_documents);
    }

    num_documents++;
  }

  // Leftover
  if (num_documents > 0)
    segment_offsets.push_back(segment_start);

  segment_offsets.push_back(std::filesystem::file_size(file_path));
  segment_docid_modifiers.push_back(num_documents);
}

void partition_file_json(const std::string &file_path, const uint64_t file_offset, const uint64_t segment_size,
                         std::vector<uint64_t> &segment_offsets,
                         std::vector<uint64_t> &segment_docid_modifiers) {
  using namespace simdjson;
  constexpr size_t BATCH_SIZE = 16 << 20;

  padded_memory_map map(file_path.c_str());
  if (!map.is_valid()) throw std::runtime_error("cannot map " + file_path);
  padded_string_view json = map.view();

  ondemand::parser parser;
  ondemand::document_stream stream;
  auto error = parser.iterate_many(json.data() + file_offset, json.size() - file_offset, BATCH_SIZE).get(stream);
  if (error) throw std::runtime_error(std::string("iterate_many: ") + error_message(error));

  uint64_t segment_start = 0;
  uint64_t num_documents = 0;
  segment_docid_modifiers.push_back(0);

  for (auto j = stream.begin(); j != stream.end(); ++j) {
    if (auto err = (*j).error())
      throw std::runtime_error(std::string("document error: ") + error_message(err));

    // current_index() is relative to the stream start (file_offset)
    const uint64_t pos = file_offset + j.current_index();
    if (num_documents == 0) {
      segment_start = pos;
    } else if (pos - segment_start > segment_size) {
      segment_offsets.push_back(segment_start);
      segment_docid_modifiers.push_back(num_documents);
      segment_start = pos;
    }
    num_documents++;
  }

  if (num_documents > 0) segment_offsets.push_back(segment_start);
  segment_offsets.push_back(json.size());
  segment_docid_modifiers.push_back(num_documents);
}

void partition_file_logfmt(const std::string &file_path, const uint64_t file_offset, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers) {
  LogfmtParser parser = LogfmtParser(file_path, file_offset, std::filesystem::file_size(file_path));

  bool first = true;
  uint64_t segment_start = 0;
  uint64_t num_documents = 0;
  segment_docid_modifiers.push_back(num_documents);
  for (auto it = parser.begin(); it != parser.end(); ++it) {
    const int64_t raw = (*it).current_offset();
    if (raw < 0)
      throw std::invalid_argument("LOGFMT Parser: Invalid offset");

    const uint64_t pos = static_cast<uint64_t>(raw);
    if (first) { 
      segment_start = pos; 
      first = false; 
    } else if (pos - segment_start > segment_size) {
      segment_offsets.push_back(segment_start);
      segment_start = static_cast<uint64_t>(pos);
      segment_docid_modifiers.push_back(num_documents);
    }

    num_documents++;
  }

  // Leftover
  if (num_documents > 0)
    segment_offsets.push_back(segment_start);

  segment_offsets.push_back(std::filesystem::file_size(file_path));
  segment_docid_modifiers.push_back(num_documents);
}

uint64_t count_documents_csv(const std::string &file_path) {
  csv::CSVFormat format;
  format.variable_columns(csv::VariableColumnPolicy::THROW);
  csv::CSVReader reader(file_path, format);

  for (const csv::CSVRow &row : reader) {}

  return reader.n_rows();
}

uint64_t count_documents_json(const std::string &file_path) {
  using namespace simdjson;
  constexpr size_t BATCH_SIZE = 16 << 20;

  padded_memory_map map(file_path.c_str());
  if (!map.is_valid()) throw std::runtime_error("cannot map " + file_path);
  padded_string_view json = map.view();
  ondemand::parser parser;
  simdjson::ondemand::document_stream stream;
  auto error = parser.iterate_many(json.data(), json.size(), BATCH_SIZE).get(stream);
  if (error) 
    throw std::runtime_error(std::string("iterate_many: ") + error_message(error));

  uint64_t num_documents = 0;
  for (auto j = stream.begin(); j != stream.end(); ++j) {
    if (auto err = (*j).error())
      throw std::runtime_error(std::string("document error: ") + error_message(err));
    num_documents++;
  }

  if (stream.truncated_bytes() > 0)
    throw std::runtime_error("trailing incomplete document");

  return num_documents;
}

uint64_t count_documents_logfmt(const std::string &file_path) {
  LogfmtParser parser = LogfmtParser(file_path);

  uint64_t num_documents = 0;
  for (auto it = parser.begin(); it != parser.end(); ++it) {
    const int64_t raw = (*it).current_offset();
    if (raw < 0)
      throw std::invalid_argument("LOGFMT Parser: Invalid offset");

    num_documents++;
  }

  return num_documents;
}