#include "search.h"
#include "fst.h"
#include "tokenizer.h"
#include "metadata.h"
#include <filesystem>
#include <regex>
#include <string>

const FieldMap search_index(const std::vector<std::string> &fields,
                            const std::string &query,
                            const Metadata &metadata) { 
  const Tokenizer tokens = Tokenizer::from_string(query);                         
  // If a range query, create a range
  Range range; // Construct dummy range
  std::pair<float, float> range_float;
  uint8_t range_type = TokenType::INVALID;
  if (is_range_query(query)) {
    range_type = get_range_query_type(tokens);

    if (range_type == TokenType::FLOAT)
      range_float = get_range_query_float(tokens, query);
    else
      range = get_range_query(tokens, query, range_type);
  }

  check_index_dir_valid(metadata.get_index_path());

  FieldMap offsets;

  // If range only need to process this, then return
  if (range_type != TokenType::INVALID) {
    if (range_type == TokenType::FLOAT)
      search_index_by_range(offsets, fields, range_float, range_type, metadata);
    else
      search_index_by_range(offsets, fields, range, range_type, metadata);
    return offsets;
  }

  for (const auto &t : tokens.alpha_tokens_q) {
    search_index_by_type(offsets, fields, t, TokenType::ALPHA, metadata);
    if (no_matches(offsets)) {
      return offsets;
    }
  }

  for (const auto t : tokens.num_tokens_q) {
    search_index_by_type(offsets, fields, t, TokenType::PINT, metadata);
    if (no_matches(offsets))
      return offsets;
  }

  for (const auto t : tokens.neg_num_tokens_q) {
    search_index_by_type(offsets, fields, t, TokenType::NINT, metadata);
    if (no_matches(offsets))
      return offsets;
  }

  for (const auto t : tokens.float_tokens_q) {
    search_index_by_type(offsets, fields, to_bytes(t), TokenType::FLOAT, metadata);
    if (no_matches(offsets))
      return offsets;
  }

  for (const auto t : tokens.d_tokens_q) {
    search_index_by_type(offsets, fields, t, TokenType::DATE, metadata);
    if (no_matches(offsets))
      return offsets;
  }

  for (const auto t : tokens.t_tokens_q) {
    search_index_by_type(offsets, fields, t, TokenType::TIME, metadata);
    if (no_matches(offsets))
      return offsets;
  }
  for (const auto t : tokens.ipv4_tokens_q) {
    search_index_by_type(offsets, fields, t, TokenType::IPV4, metadata);
    if (no_matches(offsets))
      return offsets;
  }
  return offsets;
}

const FieldMap process_queries(const std::vector<std::string> &fields,
                               const std::vector<std::string> &queries,
                               const Metadata &metadata,
                               const uint8_t query_mode) {
  FieldMap combined_queries;
  for (const auto &query : queries) {
    if (combined_queries.empty()) {
      combined_queries = search_index(fields, query, metadata);
    } else {
      switch (query_mode) {
      case QueryMode::UNION:
        combined_queries = get_entries_offsets_union(
            combined_queries, search_index(fields, query, metadata));
        break;
      case QueryMode::INTER:
        combined_queries = get_entries_offsets_intersection(
            combined_queries, search_index(fields, query, metadata));
        break;
      case QueryMode::COMP:
          combined_queries = get_entries_offsets_union(
            combined_queries, search_index(fields, query, metadata));
          break;
      default:
        throw std::invalid_argument("Query type not valid");
      }
    }
  }

  if (query_mode == QueryMode::COMP) {
    combined_queries = get_entries_offsets_compliment(combined_queries, metadata);
  }

  return combined_queries;
}

const EntryMap get_entries_offsets(const FieldMap &offsets_map,
                                   const std::string &data_path,
                                   const Metadata &metadata) {
  EntryMap all_line_offsets;
  for (const auto &[file_id, fields] : offsets_map) {
    const size_t num_segments = metadata.get_num_segments(file_id);
    std::vector<uint64_t> &line_offsets = all_line_offsets[file_id];

    size_t segment_id = 0;
    std::vector<uint32_t> entry_offsets;
    size_t loaded = SIZE_MAX;

    uint64_t local, base;
    for (const uint64_t docid : get_docids(fields)) {  // docids are sorted
      // advance to segment containing this docid
      while (segment_id + 1 < num_segments &&
             docid >= metadata.get_segment_docid_modifier(file_id, segment_id + 1))
        ++segment_id;

      if (loaded != segment_id) {
        const std::string segment_path = metadata.get_index_path() + '/' + std::to_string(file_id) +
                                         "_segment_" + std::to_string(segment_id) + ".seg";
        entry_offsets = get_entry_offsets_from_file(segment_path, metadata.get_enc_type());

        base = metadata.get_segment_offset(file_id, segment_id);
        loaded = segment_id;
      }

      local = docid - metadata.get_segment_docid_modifier(file_id, segment_id);
      line_offsets.push_back(base + entry_offsets[local]);
      line_offsets.push_back(base + entry_offsets[local + 1] - 1);
    }
  }
  return all_line_offsets;
}

const std::vector<size_t> get_file_ids_union(const FieldMap &entries_1,
                                             const FieldMap &entries_2) {
  std::vector<size_t> file_ids;
  for (const auto &[file_id, docids] : entries_1) {
    file_ids.push_back(file_id);
  }

  for (const auto &[file_id, docids] : entries_2) {
    if (entries_1.contains(file_id))
      continue;
    file_ids.push_back(file_id);
  }

  return file_ids;
}

const FieldMap get_entries_offsets_intersection(const FieldMap &entries_1,
                                                const FieldMap &entries_2) {
  FieldMap intersection_offsets;

  // Get vector of union of file_id's for both entries_1 and entries_2
  const std::vector<size_t> file_ids = get_file_ids_union(entries_1, entries_2);

  for (const auto file_id : file_ids) {
    if (!entries_1.contains(file_id) || !entries_2.contains(file_id))
      continue;

    const FieldDocids &fields = entries_1.at(file_id);
    const FieldDocids &fields_cmp = entries_2.at(file_id);

    const std::vector<uint64_t> docids = get_docid_intersection(get_docids(fields), get_docids(fields_cmp));

    FieldDocids &out = intersection_offsets[file_id];
    for (const auto &[field_id, field_docids] : fields)
      out[field_id] = get_docid_intersection(field_docids, docids);

    for (const auto &[field_id, field_docids] : fields_cmp) {
      const std::vector<uint64_t> cmp = get_docid_intersection(field_docids, docids);
      std::vector<uint64_t> merged;
      std::set_union(out[field_id].begin(), out[field_id].end(), cmp.begin(), cmp.end(),
                     std::back_inserter(merged));
      out[field_id] = std::move(merged);
    }
  }

  return intersection_offsets;
}

const FieldMap get_entries_offsets_union(const FieldMap &entries_1,
                                         const FieldMap &entries_2) {
  FieldMap union_offsets;
  // Get vector of union of file_id's for both entries_1 and entries_2
  const std::vector<size_t> file_ids = get_file_ids_union(entries_1, entries_2);

  // Offsets is a map of file_id's to offsets
  for (const auto file_id : file_ids) {
    if (!entries_1.contains(file_id) && !entries_2.contains(file_id)) {
      continue;
    } else if (!entries_1.contains(file_id)) {
      union_offsets[file_id] = entries_2.at(file_id);
      continue;
    } else if (!entries_2.contains(file_id)) {
      union_offsets[file_id] = entries_1.at(file_id);
      continue;
    }

    FieldDocids &out = union_offsets[file_id];
    out = entries_1.at(file_id);

    for (const auto &[field_id, docids_cmp] : entries_2.at(file_id)) {
      const std::vector<uint64_t> &docids = out[field_id];
      std::vector<uint64_t> merged;
      merged.reserve(docids.size() + docids_cmp.size());
      std::set_union(docids.begin(), docids.end(), docids_cmp.begin(),
                     docids_cmp.end(), std::back_inserter(merged));
      out[field_id] = std::move(merged);
    }
  }

  return union_offsets;
}

const FieldMap get_entries_offsets_difference(const FieldMap &entries_1, const FieldMap &entries_2) {
  FieldMap difference_offsets;
  // Get vector of union of file_id's for both entries_1 and entries_2
  const std::vector<size_t> file_ids = get_file_ids_union(entries_1, entries_2);

  // Offsets is a map of file_id's to offsets
  for (const auto file_id : file_ids) {
    if (!entries_1.contains(file_id)) {
      continue;
    } else if (!entries_2.contains(file_id)) {
      difference_offsets[file_id] = entries_1.at(file_id);
      continue;
    }

    const FieldDocids &fields_cmp = entries_2.at(file_id);
    FieldDocids &out = difference_offsets[file_id];

    for (const auto &[field_id, docids] : entries_1.at(file_id)) {
      auto it = fields_cmp.find(field_id);
      if (it == fields_cmp.end()) {
        out[field_id] = docids;
        continue;
      }
      std::set_difference(docids.begin(), docids.end(), it->second.begin(),
                          it->second.end(), std::back_inserter(out[field_id]));
    }
  }

  return difference_offsets;
}

const FieldMap get_entries_offsets_sym_difference(const FieldMap &entries_1, const FieldMap &entries_2) {
  FieldMap difference_offsets;

  // Get vector of union of file_id's for both entries_1 and entries_2
  const std::vector<size_t> file_ids = get_file_ids_union(entries_1, entries_2);

  // Offsets is a map of file_id's to offsets
  for (const auto file_id : file_ids) {
    if (!entries_1.contains(file_id) && !entries_2.contains(file_id)) {
      continue;
    } else if (!entries_1.contains(file_id)) {
      difference_offsets[file_id] = entries_2.at(file_id);
      continue;
    } else if (!entries_2.contains(file_id)) {
      difference_offsets[file_id] = entries_1.at(file_id);
      continue;
    }

    const FieldDocids &fields = entries_1.at(file_id);
    const FieldDocids &fields_cmp = entries_2.at(file_id);
    FieldDocids &out = difference_offsets[file_id];

    for (const auto &[field_id, docids] : fields) {
      auto it = fields_cmp.find(field_id);
      if (it == fields_cmp.end()) {
        out[field_id] = docids;
        continue;
      }
      std::set_symmetric_difference(docids.begin(), docids.end(), it->second.begin(),
                                    it->second.end(), std::back_inserter(out[field_id]));
    }

    for (const auto &[field_id, docids_cmp] : fields_cmp) {
      if (!fields.contains(field_id))
        out[field_id] = docids_cmp;
    }
  }

  return difference_offsets;
}

const FieldMap get_entries_offsets_compliment(const FieldMap &entries, const Metadata &metadata) {
  FieldMap compliment_offsets;

  const uint64_t num_files = metadata.get_file_mappings().size();

  for (uint64_t file_id = 0; file_id < num_files; ++file_id) {
    // Every doc id in this file, in order: each segment's ids shifted by its modifier
    std::vector<uint64_t> all_entries;
    const size_t num_segments = metadata.get_num_segments(file_id);
    for (size_t seg = 0; seg < num_segments; ++seg) {
      const std::string segment_path = metadata.get_index_path() + '/' + std::to_string(file_id) +
                                       "_segment_" + std::to_string(seg) + ".seg";
      const uint64_t count = get_entry_offsets_from_file(segment_path, metadata.get_enc_type()).size() - 1;
      const uint64_t modifier = metadata.get_segment_docid_modifier(file_id, seg);
      for (uint64_t i = 0; i < count; ++i)
        all_entries.push_back(modifier + i);
    }

    std::vector<uint64_t> &out = compliment_offsets[file_id][NO_FIELD];
    if (auto it = entries.find(file_id); it != entries.end()) {
      const std::vector<uint64_t> docids = get_docids(it->second);
      std::set_difference(all_entries.begin(), all_entries.end(),
                          docids.begin(), docids.end(),
                          std::back_inserter(out));
    } else {
      out = std::move(all_entries);
    }
  }

  return compliment_offsets;
}

const std::vector<uint64_t> get_docids(const FieldDocids &fields) {
  std::vector<uint64_t> docids;
  for (const auto &[field_id, field_docids] : fields)
    docids.insert(docids.end(), field_docids.begin(), field_docids.end());

  std::sort(docids.begin(), docids.end());
  docids.erase(std::unique(docids.begin(), docids.end()), docids.end());

  return docids;
}

bool no_matches(const FieldMap &offsets) {
  for (const auto &[file_id, fields] : offsets)
    for (const auto &[field_id, docids] : fields)
      if (!docids.empty()) return false;
  return true;
}

const std::vector<uint64_t>
get_docid_intersection(const std::vector<uint64_t> &docids,
                       const std::vector<uint64_t> &docids_cmp) {
  std::vector<uint64_t> intersection;
  intersection.reserve(std::min(docids.size(), docids_cmp.size()));
  std::set_intersection(docids.begin(), docids.end(), docids_cmp.begin(),
                        docids_cmp.end(), std::back_inserter(intersection));

  return intersection;
}

const std::optional<Range> get_range_id_pef(const Range &range,
                                            const std::string &offset_file_name,
                                            const uint8_t type) {
  const MappedFile mf = open_mmap_r(offset_file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();
  const auto [offset, size_to_read] = get_pef_header_details_by_type(file, file_size, type);
  
  if (!size_to_read) return std::nullopt;
  
  return find_range_pef_id(offset_file_name, offset, size_to_read, range);
}


const uint8_t get_range_query_type(const Tokenizer &tokens) {
  uint8_t type = TokenType::INVALID;
  // Alpha cannot be used in range query so throw error
  if (tokens.getAlphaTokensQ().size()) {
    std::cout <<tokens.getAlphaTokensQ().back()<<std::endl;
    throw std::invalid_argument(
        "Range query cannot contain alphabetical or float tokens");
  } else if (tokens.getNumTokensQ().size()) {
    if (type != TokenType::INVALID)
      throw std::invalid_argument("Range query must contain a single type");
    type = TokenType::PINT;
  } else if (tokens.getNegNumTokensQ().size()) {
    if (type != TokenType::INVALID)
      throw std::invalid_argument("Range query must contain a single type");
    type = TokenType::NINT;
  } else if (tokens.getFloatTokensQ().size()) {
    type = TokenType::FLOAT;
  } else if (tokens.getDateTokensQ().size()) {
    if (type != TokenType::INVALID)
      throw std::invalid_argument("Range query must contain a single type");
    type = TokenType::DATE;
  } else if (tokens.getTimeTokensQ().size()) {
    if (type != TokenType::INVALID)
      throw std::invalid_argument("Range query must contain a single type");
    type = TokenType::TIME;
  } else if (tokens.getIPTokensQ().size()) {
    if (type != TokenType::INVALID)
      throw std::invalid_argument("Range query must contain a single type");
    type = TokenType::IPV4;
  } else {
    throw std::invalid_argument("Range query invalid");
  }

  return type;
}

const Range get_range_query(const Tokenizer &tokens, const std::string &query, const uint8_t type) {
  std::vector<uint64_t> range_tokens;

  switch (type) {
  case TokenType::PINT:
    range_tokens = tokens.getNumTokensQ();
    break;
  case TokenType::NINT:
    range_tokens = tokens.getNegNumTokensQ();
    break;
  case TokenType::DATE:
    range_tokens = tokens.getDateTokensQ();
    break;
  case TokenType::TIME:
    range_tokens = tokens.getTimeTokensQ();
    break;
  case TokenType::IPV4:
    range_tokens = tokens.getIPTokensQ();
    break;
  default:
    throw std::invalid_argument("Invalid type");
  }

  if (range_tokens.size() > 2)
    throw std::invalid_argument("Range query invalid");

  // Use previous regex in is_range_query, but with 2 capture groups
  const std::regex pattern(R"(\{([0-9+\-.:]*),([0-9+\-.:]*)\})");
  std::smatch match;

  std::regex_match(query, match, pattern);

  const std::string a = match[1].str();
  const std::string b = match[2].str();
  
  // Form [a,b]
  Range range = {};
  if (a.size() && b.size()) {
    assert(range_tokens.size() == 2);
    range = {static_cast<int64_t>(range_tokens[0]),
             static_cast<int64_t>(range_tokens[1])};
    // Form [a,]
  } else if (a.size()) {
    assert(range_tokens.size() == 1);
    range = {static_cast<int64_t>(range_tokens[0]), INT32_MAX};
    // Form [,b]
  } else {
    assert(range_tokens.size() == 1);
    range = {0, static_cast<int64_t>(range_tokens[0])};
  }

  return range;
}

const std::pair<float, float> get_range_query_float(const Tokenizer &tokens, const std::string &query) {
  std::vector<float> range_tokens = tokens.getFloatTokensQ();

  if (range_tokens.size() > 2)
    throw std::invalid_argument("Range query invalid");

  // Use previous regex in is_range_query, but with 2 capture groups
  const std::regex pattern(R"(\{([0-9+\-.:]*),([0-9+\-.:]*)\})");
  std::smatch match;

  std::regex_match(query, match, pattern);

  const std::string a = match[1].str();
  const std::string b = match[2].str();                                                 
  // Form [a,b]
  float min, max;
  if (a.size() && b.size()) { 
        std::cout << range_tokens.size() << std::endl; 
    min = range_tokens[0];
    max = range_tokens[1]; 
    // Form [a,]
  } else if (a.size()) {
    assert(range_tokens.size() == 1);
    min = range_tokens[0];
    max = std::numeric_limits<float>::infinity();
    // Form [,b]
  } else {
    assert(range_tokens.size() == 1);
    min = -std::numeric_limits<float>::infinity();
    max = range_tokens[0];
  }
  
  return std::make_pair(min, max);
}

std::vector<uint32_t> get_field_ids(const std::string &field_path, const std::vector<std::string> &fields) {
  std::unique_ptr<StdCompactFst> fst = read_from_disk(field_path, get_fields_fst_pos(field_path));
  
  std::vector<uint32_t> field_ids;
  
  if (!fields.empty()) {
    for (const auto &field : fields) {
      int64_t field_id = get_id_from_fst(*fst, field);
      if (field_id != -1)
        field_ids.push_back(field_id);
      else
        throw std::runtime_error(field + " is not a valid field");
    }
  }

  std::sort(field_ids.begin(), field_ids.end());

  return field_ids;
}