#include <string>
#include <filesystem>
#include "tokenizer.h"
#include "search.h"
#include "fst.h"
#include <regex>

const FieldMap search_index(const std::string &query, const std::string &index_path, const uint8_t enc_type) {
    const Tokenizer tokens = Tokenizer::from_string(query);

    // If a range query, create a range
    std::pair<Range, uint8_t> range_data = {{-1, -1}, 0}; // Construct dummy range_data
    if (is_range_query(query))
        range_data = get_range_query(tokens, query);

    check_index_dir_valid(index_path);

    // Read in file_mappings fst
    fst::StdConstFst *fst = nullptr;
    read_from_disk(fst, index_path + "/file_mappings.fst");
    const size_t names_size = get_strings_from_fst(*fst).size();

    FieldMap offsets;

    // If range only need to process this, then return
    if (range_data.first.min != -1) {
        search_index_by_range(offsets, range_data.first, range_data.second, names_size, index_path, enc_type);
        return offsets;
    }

    for (const auto &t : tokens.getAlphaTokens()) {
        search_index_by_type(offsets, t, TokenType::ALPHA, names_size, index_path, enc_type);
    }

    for (const auto &t : tokens.getNumTokens()) {
        search_index_by_type(offsets, t, TokenType::INT, names_size, index_path, enc_type);
    }

    for (const auto &t : tokens.getFloatTokens()) {
        search_index_by_type(offsets, to_bytes(t), TokenType::FLOAT, names_size, index_path, enc_type);
    }

    for (const auto &t : tokens.getDateTokens()) {
        search_index_by_type(offsets, t, TokenType::DATE, names_size, index_path, enc_type);
    }

    for (const auto &t : tokens.getTimeTokens()) {
        search_index_by_type(offsets, t, TokenType::TIME, names_size, index_path, enc_type);
    }
    for (const auto &t : tokens.getIPTokens()) {
        search_index_by_type(offsets, t, TokenType::IPV4, names_size, index_path, enc_type);
    }

    return offsets;
}

const EntryMap process_queries(const std::vector<std::string> &queries, const std::string &data_path, const std::string &index_path, const uint8_t enc_type) {
    assert(queries.size() != 0);
    
    FieldMap intersected_queries;
    for (const auto query : queries) {
        if (!intersected_queries.size()) {
            intersected_queries = search_index(query, index_path, enc_type);
        } else {
            intersected_queries = get_entries_offsets_intersection(intersected_queries, search_index(query, index_path, enc_type));
        } 
    }

    fst::StdConstFst *fst = nullptr;
    read_from_disk(fst, index_path + "/file_mappings.fst");
    const std::vector<std::string> names = get_vec_strings_from_fst(*fst);

    const EntryMap result_entries = get_entries_offsets(intersected_queries, names, data_path, index_path, enc_type);
    
    return result_entries;
}

const EntryMap get_entries_offsets(const FieldMap &offsets_map, const std::vector<std::string> &file_names, const std::string &data_path, const std::string &index_path, const uint8_t enc_type) {
    EntryMap all_line_offsets;
    // Offsets is a map of file_id's to offsets
    for (const auto &[file_id, token_offsets] : offsets_map) {
        const std::string index_file_path = index_path + "/offsets/" + std::to_string(file_id)+ ".off";

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
    for (const auto &[file_id, docids] : entries_1) {
        if (!entries_2.contains(file_id)) continue;

        const std::vector<uint64_t> &docids_cmp = entries_2.at(file_id);
        std::vector<uint64_t> &intersect_docids = intersection_offsets[file_id];
        for (const auto &docid : docids) {
            // Check DocId present in both maps using binary search
            const auto it = std::lower_bound(docids_cmp.begin(), docids_cmp.end(), docid);
            if (it != docids_cmp.end() && *it == docid) {
                intersect_docids.push_back(docid);
            }
        }
    }

    return intersection_offsets;
}

void print_entries(const EntryMap &map, const std::string &index_path, const std::string &data_path) {
    fst::StdConstFst *fst = nullptr;
    read_from_disk(fst, index_path + "/file_mappings.fst");

    for (const auto &[file_id, entries] : map) {
        const std::string file_name = id_to_string(*fst, file_id);
        std::cout << "File: " << file_name << std::endl;
        const std::string file_path = data_path + '/' + file_name;
        
        const MappedFile mf = open_mmap_r(file_path);
        const char *file = static_cast<char*>(mf.addr());

        if (!entries.empty()) {
            for (auto it = (entries).begin(); it < (entries).end() - 1; it += 2) {
                uint64_t size = *(it + 1) - *it;
                std::cout << "Line: ";
                std::cout.write(&file[*it], size);
                std::cout << "\n";
            }
        }
    }
}

const std::optional<Range> get_range_id_pef(const Range &range, const std::string &offset_file_name, const uint8_t type) {
    const MappedFile mf = open_mmap_r(offset_file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();
    const std::pair<uint64_t, uint64_t> tok_details = get_pef_header_details_by_type(file, file_size, type); // Offset, size_to_read
    
    return find_range_pef_id(offset_file_name, tok_details.first, tok_details.second, range);
}

void search_index_by_range(FieldMap &offsets, const Range &range, const uint8_t type, const size_t files_indexed, const std::string &index_path, const uint8_t enc_type) {
    std::cout << "Range: " << range.min << " : " << range.max << std::endl;

    for (size_t i = 0; i < files_indexed; ++i) {
        const std::string offset_file_name = index_path + "/offsets/" + std::to_string(i) + ".off";
        if (!std::filesystem::exists(offset_file_name)) { 
            std::cout << offset_file_name << std::endl;
            throw std::invalid_argument("Offset file does not exist");
        }

        const std::optional<Range> ids = get_range_id_pef(range, offset_file_name, type);

        if (!ids) continue; // No token id found for current query

        const std::vector<uint64_t> id_offsets_map = get_id_offsets_from_file(offset_file_name, type, enc_type);

        offsets[i] = get_tok_offsets_from_file(*std::move(ids), id_offsets_map, offset_file_name, type, enc_type);
    }
}

const bool is_range_query(const std::string &query) {
    // Check if query in the form of a range query
    // Matches "[a,b]", where a and b are any combination of the characters [0-9+\-.:]
    const std::regex pattern(R"(\[[0-9+\-.:]*,[0-9+\-.:]*\])"); 

    return std::regex_match(query, pattern);
}

const std::pair<Range, uint8_t> get_range_query(const Tokenizer &tokens, const std::string &query) {
    // Check that tokens only contains 1-2 tokens of a single valid query type
    // Valid query types are int/date/time/ip
    std::vector<uint64_t> range_tokens;

    const size_t alpha_size = tokens.getAlphaTokens().size();
    const size_t int_size = tokens.getNumTokens().size();
    const size_t float_size = tokens.getFloatTokens().size();
    const size_t date_size = tokens.getDateTokens().size();
    const size_t time_size = tokens.getTimeTokens().size();
    const size_t ipv4_size = tokens.getIPTokens().size();

    uint8_t type;
    // Alpha and float cannot be used in range query so throw error
    if (alpha_size != 0 || float_size != 0) {
        std::cout << tokens.getAlphaTokens().front() << std::endl;
        throw std::invalid_argument("Range query cannot contain alphabetical or float tokens");
    } else if (int_size == 1 || int_size == 2) {
        range_tokens = tokens.getNumTokens();
        type = TokenType::INT;
    } else if (date_size == 1 || date_size == 2) {
        if (range_tokens.size()) throw std::invalid_argument("Range query must contain a single type");
        range_tokens = tokens.getDateTokens();
        type = TokenType::DATE;
    } else if (time_size == 1 || time_size == 2) {
        if (range_tokens.size()) throw std::invalid_argument("Range query must contain a single type");
        range_tokens = tokens.getTimeTokens();
        type = TokenType::TIME;
    } else if (ipv4_size == 1 || ipv4_size == 2) {
        if (range_tokens.size()) throw std::invalid_argument("Range query must contain a single type");
        range_tokens = tokens.getIPTokens();
        type = TokenType::IPV4;
    } else {
        throw std::invalid_argument("Range query invalid");
    }

    // Use previous regex in is_range_query, but with 2 capture groups
    const std::regex pattern(R"(\[([0-9+\-.:]*),([0-9+\-.:]*)\])"); 
    std::smatch match;

    std::regex_match(query, match, pattern);

    const std::string a = match[1].str();
    const std::string b = match[2].str();

    // Form [a,b]
    Range range = {};
    if (a.size() != 0 && b.size()) {
        assert(range_tokens.size() == 2);
        range = {static_cast<int64_t>(range_tokens[0]), static_cast<int64_t>(range_tokens[1])};
    // Form [a,]
    } else if (a.size()) {
        assert(range_tokens.size() == 1);
        range = {static_cast<int64_t>(range_tokens[0]), INT32_MAX};
    // Form [,b]
    } else {
        assert(range_tokens.size() == 1);
        range = {0, static_cast<int64_t>(range_tokens[0])};
    }

    return {range, type};
}