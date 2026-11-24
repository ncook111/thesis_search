#ifndef SEARCH_H
#define SEARCH_H

#include "index.h"
#include "segment.h"
#include "metadata.h"
#include <filesystem>
#include <algorithm>
#include <map>

inline constexpr uint32_t NO_FIELD = UINT32_MAX;

typedef std::map<uint32_t, std::vector<uint64_t>> FieldDocids;
typedef std::unordered_map<size_t, FieldDocids> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;

const FieldMap search_index(const std::vector<std::string> &fields,
                            const std::string &query,
                            const Metadata &metadata,
                            const size_t num_threads = std::max(1u, std::thread::hardware_concurrency()));
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
const std::vector<uint64_t> get_docids(const FieldDocids &fields);
bool no_matches(const FieldMap &offsets);
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
  std::unique_ptr<StdCompactFst> fst = read_from_disk(segment_path, read_pos);
  int id = get_id_from_fst(*fst, query);

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
void search_index_by_type(FieldMap &offsets, const std::vector<std::string> &fields, const Q &token, const uint8_t type, const Metadata &metadata, const size_t num_threads = std::max(1u, std::thread::hardware_concurrency())) {
  const size_t files_indexed = metadata.get_file_mappings().size();

  // Build segment tasks across all files
  std::vector<std::pair<size_t, size_t>> segment_tasks;
  std::vector<std::vector<FieldDocids>> results(files_indexed);
  std::vector<std::vector<uint32_t>> all_field_ids(files_indexed);
  for (size_t file_id = 0; file_id < files_indexed; ++file_id) {
    const size_t num_segments = metadata.get_num_segments(file_id);
    results[file_id].resize(num_segments);

    // Read in field ids from fst
    if (num_segments)
      all_field_ids[file_id] = get_field_ids(metadata.get_index_path() + '/' + std::to_string(file_id) + "_segment_0.seg", fields);

    for (size_t segment_id = 0; segment_id < num_segments; ++segment_id)
      segment_tasks.push_back({file_id, segment_id});
  }

  const size_t num_tasks = segment_tasks.size();
  const size_t n_threads = std::min<size_t>(num_tasks, num_threads);
  std::cout << "number of threads used: " << n_threads << std::endl;

  std::atomic<size_t> next{0};
  std::mutex err_mtx;
  std::exception_ptr first_error;
  std::stop_source stop;
  auto worker = [&] {
    while (!stop.stop_requested()) {
      const size_t task_id = next.fetch_add(1, std::memory_order_relaxed);
      if (task_id >= num_tasks) break;

      try {
        const auto [file_id, segment_id] = segment_tasks[task_id];
        const std::vector<uint32_t> &field_ids = all_field_ids[file_id];

        const std::string segment_path = metadata.get_index_path() + '/' + std::to_string(file_id) + "_segment_" + std::to_string(segment_id) + ".seg";
        if (!std::filesystem::exists(segment_path)) {
            throw std::invalid_argument("Segment file does not exist: " + segment_path);
        }

        const std::optional<uint64_t> id = get_id(token, segment_path, type);
        // // No token id found for current token, continue
        if (!id) continue;

        const std::vector<uint32_t> id_offsets_map = get_id_offsets_from_file(segment_path, type, metadata.get_enc_type());
        if (id_offsets_map.size()) {
          const FieldDocs offsets_cmp = get_tok_offsets_from_file(*id, id_offsets_map, field_ids, segment_path, type, metadata.get_enc_type());

          // Reconstruct actual doc_ids using per-segment modifier value
          uint64_t segment_modifier = metadata.get_segment_docid_modifier(file_id, segment_id);
          for (const auto &[field_id, field_offsets] : offsets_cmp) {
            std::vector<uint64_t> &doc_ids = results[file_id][segment_id][field_id];
            doc_ids.reserve(field_offsets.size());
            for (auto& off : field_offsets) doc_ids.push_back(off + segment_modifier);
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

  for (size_t file_id = 0; file_id < files_indexed; ++file_id) {
    // Flatten and merge into a list of offsets gathered from all segments
    FieldDocids combined_results;
    for (const auto &seg : results[file_id])
      for (const auto &[field_id, vec] : seg)
        combined_results[field_id].insert(combined_results[field_id].end(), vec.begin(), vec.end());

    // Get intersection if second token in string query, ie query such as "big mac"
    if (auto it = offsets.find(file_id); it != offsets.end()) {
      FieldDocids intersection;
      for (const auto &[field_id, docids] : it->second)
        if (auto cmp = combined_results.find(field_id); cmp != combined_results.end())
          intersection[field_id] = get_docid_intersection(docids, cmp->second);
      it->second = std::move(intersection);
    } else {
      offsets[file_id] = std::move(combined_results);
    }
  }
}

template <class Q>
void search_index_by_range(FieldMap &offsets, const std::vector<std::string> &fields, const Q &range,
                           const uint8_t type,
                           const Metadata &metadata,
                           const size_t num_threads = std::max(1u, std::thread::hardware_concurrency())) {
  if constexpr (std::is_same_v<Q, Range>) {
    std::cout << "Range: " << range.min << " : " << range.max << std::endl;
  } else {
    std::cout << "Range: " << range.first << " : " << range.second << std::endl;
  }

  const size_t files_indexed = metadata.get_file_mappings().size();

  // Build segment tasks across all files
  std::vector<std::pair<size_t, size_t>> segment_tasks;
  std::vector<std::vector<FieldDocids>> results(files_indexed);
  std::vector<std::vector<uint32_t>> all_field_ids(files_indexed);
  for (size_t file_id = 0; file_id < files_indexed; ++file_id) {
    const size_t num_segments = metadata.get_num_segments(file_id);
    results[file_id].resize(num_segments);

    // Read in field ids from fst
    if (num_segments)
      all_field_ids[file_id] = get_field_ids(metadata.get_index_path() + '/' + std::to_string(file_id) + "_segment_0.seg", fields);

    for (size_t segment_id = 0; segment_id < num_segments; ++segment_id)
      segment_tasks.push_back({file_id, segment_id});
  }

  const size_t num_tasks = segment_tasks.size();
  const size_t n_threads = std::min<size_t>(num_tasks, num_threads);
  std::cout << "number of threads used: " << n_threads << std::endl;

  std::atomic<size_t> next{0};
  std::mutex err_mtx;
  std::exception_ptr first_error;
  std::stop_source stop;

  auto worker = [&] {
    while (!stop.stop_requested()) {
      const size_t task_id = next.fetch_add(1, std::memory_order_relaxed);
      if (task_id >= num_tasks) break;

      try {
        const auto [file_id, segment_id] = segment_tasks[task_id];
        const std::vector<uint32_t> &field_ids = all_field_ids[file_id];

        const std::string segment_path = metadata.get_index_path() + '/' + std::to_string(file_id) + "_segment_" + std::to_string(segment_id) + ".seg";
        if (!std::filesystem::exists(segment_path))
          throw std::invalid_argument("Segment file does not exist: " + segment_path);

        std::optional<Range> ids;
        if constexpr (std::is_same_v<Q, Range>) {
          ids = get_range_id_pef(range, segment_path, type);
        } else {
          std::unique_ptr<StdCompactFst> fst = read_from_disk(segment_path, get_float_fst_pos(segment_path));
          ids = get_id_range_from_fst(*fst, range);
        }

        // No token id found for current query
        if (!ids) continue;

        const std::vector<uint32_t> id_offsets_map =
            get_id_offsets_from_file(segment_path, type, metadata.get_enc_type());
          
        if (id_offsets_map.empty()) continue;
        
        const FieldDocs offsets_cmp = get_tok_offsets_from_file(*ids, id_offsets_map, field_ids, segment_path, type, metadata.get_enc_type());

        // Reconstruct actual doc_ids using per-segment modifier value
        uint64_t segment_modifier = metadata.get_segment_docid_modifier(file_id, segment_id);
        for (const auto &[field_id, field_offsets] : offsets_cmp) {
          std::vector<uint64_t> &doc_ids = results[file_id][segment_id][field_id];
          doc_ids.reserve(field_offsets.size());
          for (auto& off : field_offsets) doc_ids.push_back(off + segment_modifier);
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

  for (size_t file_id = 0; file_id < files_indexed; ++file_id) {
    // Flatten and merge into a list of offsets gathered from all segments
    FieldDocids combined_results;
    for (const auto &seg : results[file_id])
      for (const auto &[field_id, vec] : seg)
        combined_results[field_id].insert(combined_results[field_id].end(), vec.begin(), vec.end());

    offsets[file_id] = std::move(combined_results);
  }
}

#endif