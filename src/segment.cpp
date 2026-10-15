#include <cstdint>
#include <fstream>
#include <cstring>
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

void partition_file(const std::string &file_path, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifers) {
  uint8_t type = str_to_file_type(file_path);
  switch (type) {
    case FileType::CSV:
      return partition_file_csv(file_path, segment_size, segment_offsets, segment_docid_modifers);
    case FileType::JSON:
      return partition_file_json(file_path, segment_size, segment_offsets, segment_docid_modifers);
    case FileType::LOGFMT:
      return partition_file_logfmt(file_path, segment_size, segment_offsets, segment_docid_modifers);
    default:
      throw std::invalid_argument("File type not supported");
  }
}

void partition_file_csv(const std::string &file_path, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers) {
  csv::CSVFormat format;
  format.variable_columns(csv::VariableColumnPolicy::THROW);
  csv::CSVReader reader(file_path, format);

  bool first = true;
  uint64_t segment_start = 0;
  uint64_t num_documents = 0;
  segment_docid_modifiers.push_back(num_documents);
  for (const csv::CSVRow &row : reader) { // Input iterator
    const uint64_t pos = row.byte_offset();

    if (first) { 
      segment_start = pos; 
      first = false; 
    } else if (pos - segment_start > segment_size) {
      segment_offsets.emplace_back(segment_start);
      segment_start = pos;
      segment_docid_modifiers.push_back(segment_docid_modifiers.back() + num_documents);
      num_documents = 0;
    }

    num_documents++;
  }

  // Leftover
  if (num_documents > 0)
    segment_offsets.emplace_back(segment_start);

  segment_offsets.emplace_back(std::filesystem::file_size(std::filesystem::path(file_path)));
}

void partition_file_json(const std::string &file_path, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers) {
  using namespace simdjson;

  constexpr size_t BATCH_SIZE = 16 << 20; // Use a 16MB batch size
  ondemand::parser parser;
  simdjson::ondemand::document_stream stream;
  padded_string json = padded_string::load(file_path);
  auto error = parser.iterate_many(json, BATCH_SIZE).get(stream);
  if (error) throw std::runtime_error(std::string("iterate_many: ") + error_message(error));

  bool first = true;
  uint64_t segment_start = 0;
  uint64_t num_documents = 0;
  segment_docid_modifiers.push_back(num_documents);
  for (auto j = stream.begin(); j != stream.end(); ++j) {
    if (auto err = (*j).error())
      throw std::runtime_error(std::string("document error: ") + error_message(err));

    const uint64_t pos = j.current_index();

    if (first) { 
      segment_start = pos; 
      first = false; 
    } else if (pos - segment_start > segment_size) {
      segment_offsets.emplace_back(segment_start);
      segment_start = pos;
      segment_docid_modifiers.push_back(segment_docid_modifiers.back() + num_documents);
      num_documents = 0;
    }

    num_documents++;
  }

  // Leftover
  if (num_documents > 0)
    segment_offsets.emplace_back(segment_start);

  segment_offsets.emplace_back(std::filesystem::file_size(std::filesystem::path(file_path)));
}

void partition_file_logfmt(const std::string &file_path, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers) {
  LogfmtParser parser = LogfmtParser(file_path);

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
      segment_offsets.emplace_back(segment_start);
      segment_start = static_cast<uint64_t>(pos);
      segment_docid_modifiers.push_back(segment_docid_modifiers.back() + num_documents);
      num_documents = 0;
    }

    num_documents++;
  }

  // Leftover
  if (num_documents > 0)
    segment_offsets.emplace_back(segment_start);

  segment_offsets.emplace_back(std::filesystem::file_size(std::filesystem::path(file_path)));
}

uint64_t count_documents(const std::string &file_path) {
  using namespace simdjson;
  int BATCH_SIZE = 16 << 20; // Use a 16MB batch size
  ondemand::parser parser;
  simdjson::ondemand::document_stream stream;
  padded_string json = padded_string::load(file_path);
  auto error = parser.iterate_many(json, BATCH_SIZE).get(stream);
  if (error) throw std::runtime_error(std::string("iterate_many: ") + error_message(error));

  uint64_t num_documents = 0;
  for (auto j = stream.begin(); j != stream.end(); ++j) {
    num_documents++;
  }

  return num_documents;
}