#include <cstdint>
#include <fstream>
#include <cstring>
#include "segment.h"

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