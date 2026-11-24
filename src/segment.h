#ifndef SEGMENT_H
#define SEGMENT_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Saved to the front of each segment file
struct Segment_header {
  uint64_t offsets_header_pos; // Position of offsets header
  uint64_t alpha_fst_pos; // FST encoded alphabetical lexicon values
  uint64_t float_fst_pos; // FST encoded float lexicon values
  uint64_t fields_fst_pos; // FST encoded field name values
};

struct Offsets_header {
  uint64_t alpha_offsets_header; // Starting offset of the alphabetical postings lists
  uint64_t alpha_id_map_header; // Maps an alphabetical tokenID to its postings list position, encoded with PEF
  uint64_t num_offsets_header; // Positive integer postings lists
  uint64_t num_id_map_header; // Maps a positive integer tokenID to its postings list position, encoded with PEF
  uint64_t neg_num_offsets_header; // Negative integer postings lists
  uint64_t neg_num_id_map_header; // Maps a negative integer tokenID to its postings list position, encoded with PEF
  uint64_t float_offsets_header; // Float postings lists
  uint64_t float_id_map_header; // Maps a float tokenID to its postings list position, encoded with PEF
  uint64_t date_offsets_header; // Date postings lists
  uint64_t date_id_map_header; // Maps a date tokenID to its postings list position, encoded with PEF
  uint64_t time_offsets_header; // Time postings lists
  uint64_t time_id_map_header; // Maps a time tokenID to its postings list position, encoded with PEF
  uint64_t ipv4_offsets_header; // IPv4 postings lists
  uint64_t ipv4_id_map_header; // Maps a ipv4 tokenID to its postings list position, encoded with PEF
  uint64_t entry_offsets_header; // Stores the starting offset of each document with respect to its original data file
  uint64_t int_toks_header; // PEF encoded lexicon values
  uint64_t neg_int_toks_header; // PEF encoded lexicon values
  uint64_t date_toks_header; // PEF encoded lexicon values
  uint64_t time_toks_header; // PEF encoded lexicon values
  uint64_t ipv4_toks_header; // PEF encoded lexicon values
};

// Includes overloads for both convential access using ifstream and memory mapped access
Offsets_header read_offsets_header(const std::string &file_name);
Offsets_header read_offsets_header(const uint8_t *file);
Segment_header get_segment_header(const std::string &file_name);
Segment_header get_segment_header(const uint8_t *file);
uint64_t get_offsets_header_pos(const std::string &file_name);
uint64_t get_offsets_header_pos(const uint8_t *file);
uint64_t get_alpha_fst_pos(const std::string &file_name);
uint64_t get_alpha_fst_pos(const uint8_t *file);
uint64_t get_float_fst_pos(const std::string &file_name);
uint64_t get_float_fst_pos(const uint8_t *file);
uint64_t get_fields_fst_pos(const std::string &file_name);
uint64_t get_fields_fst_pos(const uint8_t *file);
void update_offsets_header_pos(std::ofstream &file, const uint64_t value);
void update_alpha_fst_pos(std::ofstream &file, const uint64_t value);
void update_float_fst_pos(std::ofstream &file, const uint64_t value);
void update_fields_fst_pos(std::ofstream &file, const uint64_t value);
void partition_file(const std::string &file_path, const uint64_t file_offset, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers);
void partition_file_csv(const std::string &file_path, const uint64_t file_offset, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers);
void partition_file_json(const std::string &file_path, const uint64_t file_offset, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers);
void partition_file_logfmt(const std::string &file_path, const uint64_t file_offset, const uint64_t segment_size, std::vector<uint64_t> &segment_offsets, std::vector<uint64_t> &segment_docid_modifiers);
uint64_t count_documents_csv(const std::string &file_path);
uint64_t count_documents_json(const std::string &file_path);
uint64_t count_documents_logfmt(const std::string &file_path);

#endif