#include "metadata.h"
#include "coder.h"
#include <fstream>
#include <cstring>

Metadata Metadata::read(const std::string &index_path){
  Metadata m(index_path);

  return m;
}

Metadata Metadata::create(std::string index_path,
  std::vector<std::vector<uint64_t>> segment_offsets,
  std::vector<std::vector<uint64_t>> segment_docid_modifiers,
  StdCompactFst mappings_fst,
  const uint8_t enc_type
) {
  Metadata m(std::move(index_path), std::move(segment_offsets), std::move(segment_docid_modifiers), std::move(mappings_fst), enc_type);
  
  return m;
}

Metadata::Metadata_header Metadata::get_metadata_header() {
  return metadata_header;
}

uint8_t Metadata::get_enc_type() const {
  return metadata_header.enc_type;
}

const std::string &Metadata::get_index_path() const {
  return index_path;
}

std::vector<std::vector<uint64_t>>& Metadata::get_all_segment_offsets() {
  return all_files_segment_offsets;
}

const uint64_t Metadata::get_segment_offset(const uint64_t file_id, const uint64_t segment_id) const {
  return all_files_segment_offsets[file_id][segment_id];
}

uint64_t Metadata::get_num_segments(const uint64_t file_id) const {
  return all_files_segment_offsets[file_id].size();
}

uint64_t Metadata::get_segment_docid_modifier(const uint64_t file_id, const uint64_t segment_id) const {
  return all_files_segment_docid_modifiers[file_id][segment_id];
}

const StdCompactFst &Metadata::get_mappings_fst() const {
  return *file_mappings_fst;
}

int64_t Metadata::get_file_id(const std::string &file_name) const {
  return get_id_from_fst(*file_mappings_fst, file_name);
}

std::string Metadata::get_file_name(const size_t file_id) const {
  return id_to_string(*file_mappings_fst, file_id);
}

std::vector<std::string> Metadata::get_file_names() const {
  return get_strings_from_fst(*file_mappings_fst);
}

void Metadata::update_segment_offsets(const std::vector<std::vector<uint64_t>> &segment_offsets) {

}

void Metadata::update_file_mappings_fst(const StdCompactFst &mappings_fst) {

}

void Metadata::construct_metadata_file() {
  // Reserve space for header
  std::ofstream out(index_path + '/' + METADATA_FILE, std::ios::binary);
  out.write(reinterpret_cast<const char *>(&metadata_header), sizeof(metadata_header));

  metadata_header.all_files_segment_offsets_pos = out.tellp();
  
  std::vector<uint64_t> sol_lengths;
  sol_lengths.reserve(all_files_segment_offsets.size());

  for (const std::vector<uint64_t> &segment_offsets : all_files_segment_offsets) {
    sol_lengths.push_back(write_gvb_to_file<uint64_t>(segment_offsets, out, false));
  }

  metadata_header.segment_offsets_lengths_pos = out.tellp();

  write_gvb_to_file<uint64_t>(sol_lengths, out, false);

  metadata_header.all_files_segment_docid_modifiers_pos = out.tellp();
  
  std::vector<uint64_t> sdm_lengths;
  sdm_lengths.reserve(all_files_segment_docid_modifiers.size());

  for (const std::vector<uint64_t> &docids : all_files_segment_docid_modifiers) {
    sdm_lengths.push_back(write_gvb_to_file<uint64_t>(docids, out, false));
  }

  metadata_header.segment_docid_modifiers_lengths_pos = out.tellp();
  write_gvb_to_file<uint64_t>(sdm_lengths, out, false);

  metadata_header.file_mappings_fst_pos = out.tellp();
  write_to_disk(*file_mappings_fst, out);

  // Write full header to front of file
  out.seekp(0);
  out.write(reinterpret_cast<const char*>(&metadata_header), sizeof(metadata_header));

  out.close();

}

void Metadata::read_metadata_file() {
  std::memcpy(&metadata_header, metadata_file, sizeof(metadata_header));

  std::vector<uint64_t> sol_lengths;

  size_t size_to_read = metadata_header.all_files_segment_docid_modifiers_pos - metadata_header.segment_offsets_lengths_pos;
  sol_lengths = read_gvb_from_file<uint64_t>(metadata_file + metadata_header.segment_offsets_lengths_pos, size_to_read, false);

  uint64_t read_pos = metadata_header.all_files_segment_offsets_pos;
  for (uint64_t length : sol_lengths) {
    all_files_segment_offsets.push_back(read_gvb_from_file<uint64_t>(metadata_file + read_pos, length, false));
    read_pos += length;
  }

  std::vector<uint64_t> sdm_lengths;

  size_to_read = metadata_header.file_mappings_fst_pos - metadata_header.segment_docid_modifiers_lengths_pos;
  sdm_lengths = read_gvb_from_file<uint64_t>(metadata_file + metadata_header.segment_docid_modifiers_lengths_pos, size_to_read, false);

  read_pos = metadata_header.all_files_segment_docid_modifiers_pos;
  for (uint64_t length : sdm_lengths) {
    all_files_segment_docid_modifiers.push_back(read_gvb_from_file<uint64_t>(metadata_file + read_pos, length, false));
    read_pos += length;
  }

  file_mappings_fst.emplace(read_from_disk(index_path + '/' + METADATA_FILE, metadata_header.file_mappings_fst_pos));
}