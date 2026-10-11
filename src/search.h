#ifndef SEARCH_H
#define SEARCH_H

#include "index.h"
#include <filesystem>
#include <map>

typedef std::unordered_map<size_t, std::vector<uint32_t>> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint32_t>> EntryMap;

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
const std::vector<uint32_t>
get_docid_intersection(const std::vector<uint32_t> &docids,
                       const std::vector<uint32_t> &docids_cmp);
const std::optional<Range> get_range_id_pef(const Range &range,
                                            const std::string &offset_file_name,
                                            const uint8_t type);
const bool is_range_query(const std::string &query);
const Range get_range_query(const Tokenizer &tokens, const std::string &query,
                            const uint8_t type);
const std::pair<float, float> get_range_query_float(const Tokenizer &tokens,
                                                    const std::string &query);
const uint8_t get_range_query_type(const Tokenizer &tokens);
std::vector<uint32_t> get_field_ids(const std::string &field_path, const std::vector<std::string> &fields);

template <QueryType Q>
const std::optional<uint64_t> get_id_fst(const Q &query,
                                         const std::string &fst_file_name, const uint8_t fst_type) {
    // Read in header
  Fst_header fst_header = get_header_from_fst(fst_file_name);

  size_t read_pos;
  switch (fst_type) {
    case FstType::ALPHA:
      read_pos = fst_header.alpha_pos;
      break;
    case FstType::FLOAT:
      read_pos = fst_header.float_pos;
      break;
    case FstType::FIELDS:
      read_pos = fst_header.fields_pos;
      break;
    case FstType::OTHER:
      read_pos = 0;
      break;
    default:
      throw std::runtime_error("Fst_type not valid: " + std::to_string(fst_type));
  }

  std::unique_ptr<StdCompactFst> fst = read_from_disk(fst_file_name, read_pos);
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
const std::optional<uint32_t>
get_id(const Q &query, const std::string &offset_file_name,
       const std::string &fst_file_name, const uint8_t type) {
  if constexpr (std::is_same_v<Q, std::string> ||
                std::is_same_v<Q, std::vector<uint8_t>>) {
    if (!std::filesystem::exists(fst_file_name)) {
      std::cout << fst_file_name << std::endl;
      throw std::invalid_argument("Fst file does not exist");
    }
    return get_id_fst(query, fst_file_name, type);
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

  const size_t n_threads = 1; //std::min<size_t>(files_indexed, std::max(1u, std::thread::hardware_concurrency()));

  std::cout << "number of threads used: " << n_threads << std::endl;

  std::atomic<size_t> next{0};
  std::mutex err_mtx;
  std::exception_ptr first_error;
  std::stop_source stop;
  
  std::vector<std::optional<std::vector<uint32_t>>> results(files_indexed);

  auto worker = [&] {
    while (true) {
      const size_t i = next.fetch_add(1, std::memory_order_relaxed);
      if (i >= files_indexed) break;

      try {
        // Get list of all fst files filtered by type
        const std::string offset_file_name =
            index_path + "/offsets/" + std::to_string(i) + ".off";
        if (!std::filesystem::exists(offset_file_name)) {
            throw std::invalid_argument("Offset file does not exist: " + offset_file_name);
        }

        // Defer check for file existence until it will be used within get_id
        const std::string fst_file_name = index_path + "/fsts/" + std::to_string(i) + ".fst";
        const std::optional<uint64_t> id = get_id(token, offset_file_name, fst_file_name, type);
        // // No token id found for current token, clear list of offsets and continue
        if (!id) {
            results[i].emplace();
            continue;
        }

        // Read in field ids from fst
        const std::vector<uint32_t> field_ids = get_field_ids(index_path + "/fsts/" + std::to_string(i) + ".fst", fields);

        const std::vector<uint32_t> id_offsets_map = get_id_offsets_from_file(offset_file_name, type, enc_type);
        if (id_offsets_map.size()) {
          std::vector<uint32_t> offsets_cmp = get_tok_offsets_from_file(*id, id_offsets_map, field_ids, offset_file_name, type, enc_type);
          // Get intersection if second token in string query, ie query such as "big mac"
          if (auto it = offsets.find(i); it != offsets.end()) {
            results[i] = get_docid_intersection(it->second, offsets_cmp);
          } else {
            results[i] = std::move(offsets_cmp);
          }
        }
      } catch (...) {
        std::lock_guard lk(err_mtx);
        if (!first_error) first_error = std::current_exception();
        stop.request_stop();
      } 
    }
  };
  
  {
    std::vector<std::jthread> pool;
    pool.reserve(n_threads);
    for (size_t t = 0; t < n_threads; ++t) pool.emplace_back(worker);
  }

  if (first_error) std::rethrow_exception(first_error);

  // Merge into offsets
  for (size_t i = 0; i < files_indexed; ++i)
    if (results[i]) offsets[i] = std::move(*results[i]);
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

  const size_t n_threads = std::min<size_t>(files_indexed, std::max(1u, std::thread::hardware_concurrency()));

  std::cout << "number of threads used: " << n_threads << std::endl;

  std::atomic<size_t> next{0};
  std::mutex err_mtx;
  std::exception_ptr first_error;
  std::stop_source stop;
  
  std::vector<std::optional<std::vector<uint32_t>>> results(files_indexed);

  auto worker = [&] {
    while (true) {
      const size_t i = next.fetch_add(1, std::memory_order_relaxed);
      if (i >= files_indexed) break;

      try {
        const std::string offset_file_name =
            index_path + "/offsets/" + std::to_string(i) + ".off";
        if (!std::filesystem::exists(offset_file_name))
          throw std::invalid_argument("Offset file does not exist: " + offset_file_name);

        std::optional<Range> ids;
        if constexpr (std::is_same_v<Q, Range>) {
          ids = get_range_id_pef(range, offset_file_name, type);
        } else {
          Fst_header fst_header = get_header_from_fst(index_path + "/fsts/" + std::to_string(i) + ".fst");
          std::unique_ptr<StdCompactFst> fst = read_from_disk(index_path + "/fsts/" + std::to_string(i) + ".fst", fst_header.float_pos);
          ids = get_id_range_from_fst(*fst, range);
        }
        
         // No token id found for current query
        if (!ids) {
          results[i].emplace();
          continue;
        }

        // Read in field ids from fst
        const std::vector<uint32_t> field_ids = get_field_ids(index_path + "/fsts/" + std::to_string(i) + ".fst", fields);

        const std::vector<uint32_t> id_offsets_map =
            get_id_offsets_from_file(offset_file_name, type, enc_type);
       results[i] = get_tok_offsets_from_file(*ids, id_offsets_map, field_ids, offset_file_name, type, enc_type);
      } catch (...) {
        std::lock_guard lk(err_mtx);
        if (!first_error) first_error = std::current_exception();
        stop.request_stop();
      } 
    }
  };

  {
    std::vector<std::jthread> pool;
    pool.reserve(n_threads);
    for (size_t t = 0; t < n_threads; ++t) pool.emplace_back(worker);
  }

  if (first_error) std::rethrow_exception(first_error);

  // Merge into offsets
  for (size_t i = 0; i < files_indexed; ++i)
    if (results[i]) offsets[i] = std::move(*results[i]);
}

#endif