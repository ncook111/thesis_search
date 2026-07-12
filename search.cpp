#include <string>
#include <filesystem>
#include "tokenizer.h"
#include "index.h"
#include "search.h"
#include "fst.h"

const FieldMap search_index(const std::string &query, const std::string &index_path, const uint8_t enc_type) {
    const Tokenizer tokens = Tokenizer::from_string(query);

    // Check index/fsts directory exists
    if (!std::filesystem::exists(index_path + "/fsts")) {
        throw std::invalid_argument("Fst's directory does not exist");
    }

    // Check index/fsts directory exists
    if (!std::filesystem::exists(index_path + "/offsets")) {
        throw std::invalid_argument("Offset's directory does not exist");
    }

    // Check file_mappings file exists
    if (!std::filesystem::exists(index_path + "/file_mappings.fst")) {
        throw std::invalid_argument("file_mappings file does not exist");
    }

    // Read in file_mappings fst
    fst::StdConstFst *fst = nullptr;
    read_from_disk(fst, index_path + "/file_mappings.fst");
    const std::set<std::string> names = get_strings_from_fst(*fst);
    const size_t names_size = names.size();

    FieldMap offsets;

    for (const auto &t : tokens.alpha_tokens) {
        search_index_by_type(offsets, t.first, TokenType::ALPHA, names_size, index_path, enc_type);
    }

    for (const auto &t : tokens.num_tokens) {
        search_index_by_type(offsets, t.first, TokenType::INT, names_size, index_path, enc_type);
    }

    for (const auto &t : tokens.float_tokens) {
        search_index_by_type(offsets, to_bytes(t.first), TokenType::FLOAT, names_size, index_path, enc_type);
    }

    for (const auto &t : tokens.d_tokens) {
        search_index_by_type(offsets, t.first, TokenType::DATE, names_size, index_path, enc_type);
    }

    for (const auto &t : tokens.t_tokens) {
        search_index_by_type(offsets, t.first, TokenType::TIME, names_size, index_path, enc_type);
    }
    for (const auto &t : tokens.ipv4_tokens) {
        search_index_by_type(offsets, t.first, TokenType::IPV4, names_size, index_path, enc_type);
    }

    return offsets;
}

const EntryMap process_queries(const std::vector<std::string> &queries, const std::string &data_path, const std::string &index_path, const uint8_t enc_type) {
    assert(queries.size() != 0);

    fst::StdConstFst *fst = nullptr;
    read_from_disk(fst, index_path + "/file_mappings.fst");
    const std::vector<std::string> names = get_vec_strings_from_fst(*fst);
    
    FieldMap intersected_queries = search_index(queries.front(), index_path, enc_type);
    for (auto it = queries.begin() + 1; it != queries.end(); ++it) {
        intersected_queries = get_entries_offsets_intersection(intersected_queries, search_index(*it, index_path, enc_type));
    }

    const EntryMap result_entries = get_entries_offsets(intersected_queries, names, data_path, index_path, enc_type);

    return result_entries;
}

const EntryMap get_entries_offsets(const FieldMap &offsets_map, const std::vector<std::string> &file_names, const std::string &data_path, const std::string &index_path, const uint8_t enc_type) {
    EntryMap all_line_offsets;
    // Offsets is a map of file_id's to offsets
    for (const auto &o : offsets_map) {
        const uint64_t file_id = o.first;
        const std::string index_file_path = index_path + "/offsets/" + std::to_string(file_id)+ ".off";
        const std::vector<uint64_t> &token_offsets = o.second;

        // All entry offsets for current data file:
        const std::vector<uint64_t> entry_offsets = get_entry_offsets_from_file(index_file_path, enc_type);

        // For each token_offset, get closest prev newline and match in file to get offset to end of line
        std::vector<uint64_t> &line_offsets = all_line_offsets[file_id];
        for (const auto &cur : token_offsets) {
            line_offsets.push_back(entry_offsets[cur]);
            line_offsets.push_back(entry_offsets[cur + 1] - 1);
        }
    }

    return all_line_offsets;
}

const FieldMap get_entries_offsets_intersection(const FieldMap &entries_1, const FieldMap &entries_2) {
    FieldMap intersection_offsets;
    
    // Offsets is a map of file_id's to offsets
    for (const auto &o : entries_1) {
        const size_t file_id = o.first;
        if (!entries_2.contains(file_id)) continue;

        const std::vector<uint64_t> &docids_1 = o.second;
        const std::vector<uint64_t> &docids_2 = entries_2.at(file_id);
        std::vector<uint64_t> &intersect_docids = intersection_offsets[file_id];
        for (const auto &docid : docids_1) {
            // Check DocId present in both maps using binary search
            const auto it = std::lower_bound(docids_2.begin(), docids_2.end(), docid);
            if (it != docids_2.end() && *it == docid) {
                intersect_docids.push_back(docid);
            }
        }
    }

    return intersection_offsets;
}

void print_entries(const EntryMap &map, const std::string &index_path, const std::string &data_path) {
    fst::StdConstFst *fst = nullptr;
    read_from_disk(fst, index_path + "/file_mappings.fst");

    for (const auto &i : map) {
        const std::string file_name = id_to_string(*fst, i.first);
        std::cout << "File: " << file_name << std::endl;
        const uint64_t file_id = i.first;
        const std::string file_path = data_path + '/' + file_name;
        const MappedFile mf = open_mmap_r(file_path);
        const char *file = static_cast<char*>(mf.addr());
        const size_t file_size = mf.size();

        for (auto it = (i.second).begin(); it < (i.second).end() - 1; it += 2) {
            uint64_t size = *(it + 1) - *it;
            std::cout << "Line: ";
            std::cout.write(&file[*it], size);
            std::cout << "\n";
        }
    }
}