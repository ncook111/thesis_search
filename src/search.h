#ifndef SEARCH_H
#define SEARCH_H

#include "index.h"
#include "segment.h"
#include "metadata.h"
#include <filesystem>
#include <map>

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;

const FieldMap search_index(const std::vector<std::string> &fields,
                            const std::string &query,
                            const Metadata &metadata);
const FieldMap process_queries(const std::vector<std::string> &fields,
                               const std::vector<std::string> &queries,
                               const Metadata &metadata,
                               const uint8_t query_mode);
const std::vector<size_t> get_file_ids_union(const FieldMap &entries_1,
                                             const FieldMap &entries_2);
const EntryMap get_entries_offsets(const FieldMap &offsets_map,
                                   const std::string &data_path,
                                   const Metadata &metadata);
const FieldMap get_entries_offsets_intersection(const FieldMap &entries_1,
                                                const FieldMap &entries_2);
const FieldMap get_entries_offsets_union(const FieldMap &entries_1,
                                         const FieldMap &entries_2);
const FieldMap get_entries_offsets_difference(const FieldMap &entries_1, const FieldMap &entries_2);
const FieldMap get_entries_offsets_sym_difference(const FieldMap &entries_1, const FieldMap &entries_2);
const FieldMap get_entries_offsets_compliment(const FieldMap &entries, const Metadata &metadata);
const std::vector<uint64_t>
get_docid_intersection(const std::vector<uint64_t> &docids,
                       const std::vector<uint64_t> &docids_cmp);
const std::optional<Range> get_range_id_pef(const Range &range,
                                            const std::string &segment_path,
                                            const uint8_t type);
const bool is_range_query(const std::string &query);
const Range get_range_query(const Tokenizer &tokens, const std::string &query,
                            const uint8_t type);
const std::pair<float, float> get_range_query_float(const Tokenizer &tokens,
                                                    const std::string &query);
const uint8_t get_range_query_type(const Tokenizer &tokens);
std::vector<uint32_t> get_field_ids(const std::string &field_path, const std::vector<std::string> &fields);

template <QueryType Q>
const std::optional<uint64_t> get_id_fst(const Q &query, const std::string &segment_path, const uint8_t token_type) {
  
  const uint8_t fst_type = (token_type == TokenType::FLOAT) ? FstType::FLOAT : FstType::ALPHA;
  size_t read_pos = get_fst_pos(segment_path, fst_type);
  StdCompactFst fst = read_from_disk(segment_path, read_pos);
  int id = get_id_from_fst(fst, query);

  return (id >= 0) ? std::make_optional(id) : std::nullopt;
}

template <QueryType Q>
const std::optional<uint64_t> get_id_pef(const Q &query,
                                         const std::string &segment_path,
                                         const uint8_t type) {
  const MappedFile mf = open_mmap_r(segment_path);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();
  const auto [offset, size_to_read] = get_pef_header_details_by_type(file, file_size, type);
  
  if (size_to_read)
    return find_pef_id(segment_path, offset, size_to_read, query);
  else
    return std::nullopt;
}

template <QueryType Q>
const std::optional<uint64_t>
get_id(const Q &query, const std::string &segment_path, const uint8_t type) {
  if constexpr (std::is_same_v<Q, std::string> ||
                std::is_same_v<Q, std::vector<uint8_t>>) {
    if (!std::filesystem::exists(segment_path)) {
      std::cout << segment_path << std::endl;
      throw std::invalid_argument("Fst file does not exist");
    }
    return get_id_fst(query, segment_path, type);
  } else {
    return get_id_pef(query, segment_path, type);
  }
}

template <QueryType Q>
void search_index_by_type(FieldMap &offsets, const std::vector<std::string> &fields, const Q &token, const uint8_t type, const Metadata &metadata) {
  const size_t files_indexed = metadata.get_file_names().size();
  
  for (size_t file_id = 0; file_id < files_indexed; ++file_id) {
    std::cout << "Fileid: " << file_id << std::endl;
    const size_t num_segments = metadata.get_num_segments(file_id);
    std::vector<std::vector<uint64_t>> results(num_segments);

    std::cout << "num_segments: " <<num_segments << std::endl;

    const size_t n_threads = std::min<size_t>(num_segments, std::max(1u, std::thread::hardware_concurrency()));
    std::cout << "number of threads used: " << n_threads << std::endl;

    std::atomic<size_t> next{0};
    std::mutex err_mtx;
    std::exception_ptr first_error;
    std::stop_source stop;
    auto worker = [&] {
      while (!stop.stop_requested()) {
        const size_t segment_id = next.fetch_add(1, std::memory_order_relaxed);
        if (segment_id >= num_segments) break;

        try {
          const std::string segment_path = metadata.get_index_path() + '/' + std::to_string(file_id) + "_segment_" + std::to_string(segment_id) + ".seg";
          if (!std::filesystem::exists(segment_path)) {
              throw std::invalid_argument("Segment file does not exist: " + segment_path);
          }

          const std::optional<uint64_t> id = get_id(token, segment_path, type);
          // // No token id found for current token, continue
          if (!id) continue;

          // Read in field ids from fst
          const std::vector<uint32_t> field_ids = get_field_ids(segment_path, fields);

          const std::vector<uint32_t> id_offsets_map = get_id_offsets_from_file(segment_path, type, metadata.get_enc_type());
          if (id_offsets_map.size()) {
            std::vector<uint32_t> offsets_cmp = get_tok_offsets_from_file(*id, id_offsets_map, field_ids, segment_path, type, metadata.get_enc_type());

            // Reconstruct actual doc_ids using per-segment modifier value
            uint64_t segment_modifier = metadata.get_segment_docid_modifier(file_id, segment_id);
            std::vector<uint64_t> doc_ids;
            doc_ids.reserve(offsets_cmp.size());
            for (auto& off : offsets_cmp) doc_ids.push_back(off + segment_modifier);

            results[segment_id] = std::move(doc_ids);
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

    // Flatten and merge into a list of offsets gathered from all segments
    std::vector<uint64_t> combined_results;
    for (const auto &vec : results)
      combined_results.insert(combined_results.end(), vec.begin(), vec.end());

    // Get intersection if second token in string query, ie query such as "big mac"
    if (auto it = offsets.find(file_id); it != offsets.end()) {
      offsets[file_id] = get_docid_intersection(it->second, combined_results);
    } else {
      offsets[file_id] = std::move(combined_results);
    }
  }
}

template <class Q>
void search_index_by_range(FieldMap &offsets, const std::vector<std::string> &fields, const Q &range,
                           const uint8_t type,
                           const Metadata &metadata) {
  if constexpr (std::is_same_v<Q, Range>) {
    std::cout << "Range: " << range.min << " : " << range.max << std::endl;
  } else {
    std::cout << "Range: " << range.first << " : " << range.second << std::endl;
  }

  const size_t files_indexed = metadata.get_file_names().size();

  for (size_t file_id = 0; file_id < files_indexed; ++file_id) {
    const size_t num_segments = metadata.get_num_segments(file_id);
    std::vector<std::vector<uint64_t>> results(num_segments);

    const size_t n_threads = std::min<size_t>(num_segments, std::max(1u, std::thread::hardware_concurrency()));
    std::cout << "number of threads used: " << n_threads << std::endl;

    std::atomic<size_t> next{0};
    std::mutex err_mtx;
    std::exception_ptr first_error;
    std::stop_source stop;

    auto worker = [&] {
      while (!stop.stop_requested()) {
        const size_t segment_id = next.fetch_add(1, std::memory_order_relaxed);
        if (segment_id >= num_segments) break;

        try {
          const std::string segment_path = metadata.get_index_path() + '/' + std::to_string(file_id) + "_segment_" + std::to_string(segment_id) + ".seg";
          if (!std::filesystem::exists(segment_path))
            throw std::invalid_argument("Segment file does not exist: " + segment_path);

          std::optional<Range> ids;
          if constexpr (std::is_same_v<Q, Range>) {
            ids = get_range_id_pef(range, segment_path, type);
          } else {
            StdCompactFst fst = read_from_disk(segment_path, get_float_fst_pos(segment_path));
            ids = get_id_range_from_fst(fst, range);
          }

          // No token id found for current query
          if (!ids) continue;

          // Read in field ids from fst
          const std::vector<uint32_t> field_ids = get_field_ids(segment_path, fields);

          const std::vector<uint32_t> id_offsets_map =
              get_id_offsets_from_file(segment_path, type, metadata.get_enc_type());
          const std::vector<uint32_t> offsets_cmp = get_tok_offsets_from_file(*ids, id_offsets_map, field_ids, segment_path, type, metadata.get_enc_type());

          // Reconstruct actual doc_ids using per-segment modifier value
          uint64_t segment_modifier = metadata.get_segment_docid_modifier(file_id, segment_id);
          std::vector<uint64_t> doc_ids;
          doc_ids.reserve(offsets_cmp.size());
          for (auto& off : offsets_cmp) doc_ids.push_back(off + segment_modifier);

          results[segment_id] = std::move(doc_ids);
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

    // Flatten and merge into a list of offsets gathered from all segments
    std::vector<uint64_t> combined_results;
    for (const auto &vec : results)
      combined_results.insert(combined_results.end(), vec.begin(), vec.end());

    offsets[file_id] = std::move(combined_results);
  }
}
#endif