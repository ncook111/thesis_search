#ifndef SEARCH_H
#define SEARCH_H

#include <map>
#include "index.h"

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;


const FieldMap search_index(const std::string &query, const std::string &index_path, const uint8_t enc_type);
const EntryMap process_queries(const std::vector<std::string> &queries, const std::string &data_path, const std::string &index_path, const uint8_t enc_type);
const EntryMap get_entries_offsets(const FieldMap &offsets_map, const std::vector<std::string> &file_names, const std::string &data_path, const std::string &index_path, const uint8_t enc_type);
const FieldMap get_entries_offsets_intersection(const FieldMap &entries_1, const FieldMap &entries_2);
void print_entries(const EntryMap &map, const std::string &index_path, const std::string &data_path);
const std::vector<uint64_t> get_range_id_pef(Range range, const std::string &offset_file_name, const uint8_t type);
void search_index_by_range(FieldMap &offsets, const Range &range, const uint8_t type, const size_t files_indexed, const std::string &index_path, const uint8_t enc_type);
const bool is_range_query(const std::string &query);
const std::pair<Range, uint8_t> get_range_query(const Tokenizer &tokens, const std::string &query);

template <QueryType Q>
const std::optional<uint64_t> get_id_fst(const Q &query, const std::string &fst_file_name) {
    fst::StdConstFst *fst = nullptr;
    read_from_disk(fst, fst_file_name);

    return get_id_from_fst(*fst, query);
}

template <QueryType Q>
const std::optional<uint64_t> get_id_pef(const Q &query, const std::string &offset_file_name, const uint8_t type) {
    const MappedFile mf = open_mmap_r(offset_file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();
    const std::pair<uint64_t, uint64_t> tok_details = get_pef_header_details_by_type(file, file_size, type); // Offset, size_to_read
    
    return find_pef_id(offset_file_name, tok_details.first, tok_details.second, query);
}

template <QueryType Q>
const std::optional<uint64_t> get_id(const Q &query, const std::string &offset_file_name, const std::string &fst_file_name, const uint8_t type) {
    if constexpr (std::is_same_v<Q, std::string> || std::is_same_v<Q, std::vector<uint8_t>>) {
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
void search_index_by_type(FieldMap &offsets, const Q &query, const uint8_t type, size_t files_indexed, const std::string &index_path, const uint8_t enc_type) {
    const std::string type_str = token_type_to_str(type);

    // Get list of all fst files filtered by type
    for (size_t i = 0; i < files_indexed; ++i) {
        const std::string offset_file_name = index_path + "/offsets/" + std::to_string(i) + ".off";
        if (!std::filesystem::exists(offset_file_name)) { 
            std::cout << offset_file_name << std::endl;
            throw std::invalid_argument("Offset file does not exist");
        }

        // Defer check for file existence until it will be used within get_id
        const std::string fst_file_name = index_path + "/fsts/" + std::to_string(i) + '_' + type_str + ".fst";

        const std::optional<uint64_t> id = get_id(query, offset_file_name, fst_file_name, type);

        if (!id) continue; // No token id found for current query

        const std::vector<uint64_t> id_offsets_map = get_id_offsets_from_file(offset_file_name, type, enc_type);

        if (id_offsets_map.size()) {
            offsets[i] = get_tok_offsets_from_file(*std::move(id), id_offsets_map, offset_file_name, type, enc_type);
        }
    }
}

#endif