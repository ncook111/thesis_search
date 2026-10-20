#ifndef METADATA_H
#define METADATA_H

#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include "helper.h"

static const std::string METADATA_FILE = ".metadata";

class Metadata {
  struct Metadata_header {
    uint8_t enc_type;
    uint64_t all_files_segment_offsets_pos; 
    uint64_t segment_offsets_lengths_pos;
    uint64_t all_files_segment_docid_modifiers_pos;
    uint64_t segment_docid_modifiers_lengths_pos;
    uint64_t file_mappings_pos;
  };

  public:
    Metadata() = default;

    static Metadata read(const std::string &index_path);
    static Metadata create(
      std::string index_path,
      std::vector<std::vector<uint64_t>> segment_offsets,
      std::vector<std::vector<uint64_t>> segment_docid_modifiers,
      std::vector<std::string> file_mappings,
      const uint8_t enc_type
    );
    Metadata_header get_metadata_header();
    uint8_t get_enc_type() const;
    std::vector<std::vector<uint64_t>> get_all_segment_offsets() const;
    std::vector<std::vector<uint64_t>> get_all_segment_docid_modifiers() const;
    const std::string &get_index_path() const;
    const uint64_t get_segment_offset(const uint64_t file_id, const uint64_t segment_id) const;
    uint64_t get_num_segments(const uint64_t file_id) const;
    uint64_t get_segment_docid_modifier(const uint64_t file_id, const uint64_t segment_id) const;
    uint64_t get_num_documents(const uint64_t file_id) const;
    int64_t get_file_id(const std::string &file_name) const;
    uint64_t get_file_size(const uint64_t file_id) const;
    const std::string &get_file_name(const size_t file_id) const;
    const std::vector<std::string> &get_file_mappings() const;
    void update_segment_offsets(std::vector<std::vector<uint64_t>> segment_offsets);
    void update_segment_docid_modifiers(std::vector<std::vector<uint64_t>> segment_docid_modifiers);
    void update_file_mappings(std::vector<std::string> file_mappings);
    void update_metadata_file(std::vector<std::vector<uint64_t>> segment_offsets, std::vector<std::vector<uint64_t>> segment_docid_modifiers, std::vector<std::string> file_mappings);

  private:
    std::string index_path;
    Metadata_header metadata_header;
    std::vector<std::vector<uint64_t>> all_files_segment_offsets;
    std::vector<std::vector<uint64_t>> all_files_segment_docid_modifiers;
    std::vector<std::string> file_mappings;
    MappedFile mf;
    uint8_t* metadata_file;
    size_t metadata_file_size;

    Metadata(const std::string &index_path)
    : index_path(index_path),
      metadata_header{},
      mf(open_mmap_r(index_path + '/' + METADATA_FILE)),
      metadata_file(static_cast<uint8_t *>(mf.addr())),
      metadata_file_size(mf.size())
    {
      read_metadata_file();
    }

    Metadata(std::string i_path, std::vector<std::vector<uint64_t>> segment_offsets, std::vector<std::vector<uint64_t>> segment_docid_modifiers, std::vector<std::string> file_mappings, const uint8_t enc_type)
    : index_path(std::move(i_path)),
      metadata_header({enc_type, 0, 0, 0, 0, 0}),
      all_files_segment_offsets(std::move(segment_offsets)),
      all_files_segment_docid_modifiers(std::move(segment_docid_modifiers)),
      file_mappings(std::move(file_mappings)),
      mf((construct_metadata_file(), open_mmap_r(index_path + '/' + METADATA_FILE))),
      metadata_file(static_cast<uint8_t *>(mf.addr())),
      metadata_file_size(mf.size()) {}

    void construct_metadata_file();
    void read_metadata_file();
};

#endif