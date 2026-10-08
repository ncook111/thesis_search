#ifndef SEARCH_H
#define SEARCH_H

#include "index.h"
#include <filesystem>
#include <map>

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;

const FieldMap search_index(const std::vector<std::string> &fields,
                            const std::string &query,
                            const std::string &index_path,
                            const uint8_t enc_type);
const FieldMap process_queries(const std::vector<std::string> &fields,
                               const std::vector<std::string> &queries,
                               const std::string &index_path,
                               const uint8_t enc_type,
                               const uint8_t query_mode);
const std::vector<size_t> get_file_ids_union(const FieldMap &entries_1,
                                             const FieldMap &entries_2);
const EntryMap get_entries_offsets(const FieldMap &offsets_map,
                                   const std::vector<std::string> &file_names,
                                   const std::string &data_path,
                                   const std::string &index_path,
                                   const uint8_t enc_type);
const FieldMap get_entries_offsets_intersection(const FieldMap &entries_1,
                                                const FieldMap &entries_2);
const FieldMap get_entries_offsets_union(const FieldMap &entries_1,
                                         const FieldMap &entries_2);
const FieldMap get_entries_offsets_difference(const FieldMap &entries_1, const FieldMap &entries_2);
const FieldMap get_entries_offsets_sym_difference(const FieldMap &entries_1, const FieldMap &entries_2);
const FieldMap get_entries_offsets_compliment(const FieldMap &entries, const std::string &index_path, const uint8_t enc_type);
const std::vector<uint64_t>
get_docid_intersection(const std::vector<uint64_t> &docids,
                       const std::vector<uint64_t> &docids_cmp);
const std::optional<Range> get_range_id_pef(const Range &range,
                                            const std::string &offset_file_name,
                                            const uint8_t type);
const bool is_range_query(const std::string &query);
const Range get_range_query(const Tokenizer &tokens, const std::string &query,
                            const uint8_t type);
const std::pair<float, float> get_range_query_float(const Tokenizer &tokens,
                                                    const std::string &query);
const uint8_t get_range_query_type(const Tokenizer &tokens);
std::vector<uint64_t> get_field_ids(const std::string &field_path, const std::vector<std::string> &fields);

template <QueryType Q>
const std::optional<uint64_t> get_id_fst(const Q &query,
                                         const std::string &fst_file_name) {
  std::unique_ptr<StdCompactFst> fst = read_from_disk(fst_file_name);
  int id = get_id_from_fst(*fst, query);

  return (id >= 0) ? std::make_optional(id) : std::nullopt;
}

template <QueryType Q>
const std::optional<uint64_t> get_id_pef(const Q &query,
                                         const std::string &offset_file_name,
                                         const uint8_t type) {
  const MappedFile mf = open_mmap_r(offset_file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();
  const auto [offset, size_to_read] = get_pef_header_details_by_type(file, file_size, type);
  
  if (size_to_read)
    return find_pef_id(offset_file_name, offset, size_to_read, query);
  else
    return std::nullopt;
}

template <QueryType Q>
const std::optional<uint64_t>
get_id(const Q &query, const std::string &offset_file_name,
       const std::string &fst_file_name, const uint8_t type) {
  if constexpr (std::is_same_v<Q, std::string> ||
                std::is_same_v<Q, std::vector<uint8_t>>) {
    if (!std::filesystem::exists(fst_file_name)) {
      std::cout << fst_file_name << std::endl;
      throw std::invalid_argument("Fst file does not exist");
    }
    return get_id_fst(query, fst_file_name);
  } else {
    return get_id_pef(query, offset_file_name, type);
  }
}

template <QueryType Q>
void search_index_by_type(FieldMap &offsets, const std::vector<std::string> &fields, const Q &token, const uint8_t type,
                          const size_t files_indexed,
                          const std::string &index_path,
                          const uint8_t enc_type) {
  const std::string type_str = token_type_to_str(type);

  // Get list of all fst files filtered by type
  for (size_t i = 0; i < files_indexed; ++i) {
    const std::string offset_file_name =
        index_path + "/offsets/" + std::to_string(i) + ".off";
    if (!std::filesystem::exists(offset_file_name)) {
        std::cout << offset_file_name << std::endl;
        throw std::invalid_argument("Offset file does not exist");
    }

    // Defer check for file existence until it will be used within get_id
    const std::string fst_file_name = index_path + "/fsts/" + std::to_string(i) + '_' + type_str + ".fst";
    const std::optional<uint64_t> id = get_id(token, offset_file_name, fst_file_name, type);
    // // No token id found for current token, clear list of offsets and continue
    if (!id) {
        offsets[i].clear();
        continue;
    }

    // Read in field ids from fst
    const std::vector<uint64_t> field_ids = get_field_ids(index_path + "/fsts/" + std::to_string(i) + "_fields.fst", fields);

    const std::vector<uint64_t> id_offsets_map = get_id_offsets_from_file(offset_file_name, type, enc_type);
    if (id_offsets_map.size()) {
      std::vector<uint64_t> offsets_cmp = get_tok_offsets_from_file(*std::move(id), id_offsets_map, field_ids, offset_file_name, type, enc_type);
      // Get intersection if second token in string query, ie query such as "big mac"
      if (offsets.contains(i)) {
        offsets[i] = get_docid_intersection(offsets[i], offsets_cmp);
      } else {
        offsets[i] = std::move(offsets_cmp);
      }
    }

  }
}

template <class Q>
void search_index_by_range(FieldMap &offsets, const std::vector<std::string> &fields, const Q &range,
                           const uint8_t type, const size_t files_indexed,
                           const std::string &index_path,
                           const uint8_t enc_type) {
  if constexpr (std::is_same_v<Q, Range>) {
    std::cout << "Range: " << range.min << " : " << range.max << std::endl;
  } else {
    std::cout << "Range: " << range.first << " : " << range.second << std::endl;
  }
  for (size_t i = 0; i < files_indexed; ++i) {
    const std::string offset_file_name =
        index_path + "/offsets/" + std::to_string(i) + ".off";
    if (!std::filesystem::exists(offset_file_name)) {
      std::cout << offset_file_name << std::endl;
      throw std::invalid_argument("Offset file does not exist");
    }

    std::optional<Range> ids;
    if constexpr (std::is_same_v<Q, Range>) {
      ids = get_range_id_pef(range, offset_file_name, type);
    } else {
      std::unique_ptr<StdCompactFst> fst = read_from_disk(index_path + "/fsts/" + std::to_string(i) + "_float.fst");
      ids = get_id_range_from_fst(*fst, range);
    }

    if (!ids)
      continue; // No token id found for current query

    // Read in field ids from fst
    const std::vector<uint64_t> field_ids = get_field_ids(index_path + "/fsts/" + std::to_string(i) + "_fields.fst", fields);

    const std::vector<uint64_t> id_offsets_map =
        get_id_offsets_from_file(offset_file_name, type, enc_type);
    offsets[i] = get_tok_offsets_from_file(*std::move(ids), id_offsets_map, field_ids,
                                           offset_file_name, type, enc_type);
  }
}

#endif