#include <vector>
#include <set>
#include <fstream>
#include <map>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include "coder.h"
#include "index.h"
#include "fst.h"
#include "helper.h"

bool valid_extension(const std::string &ext) {
    return (
        ext == ".csv" ||
        ext == ".json" ||
        ext == ".logfmt"
    );
}

void create_index(const std::string &index_dir_str, const std::string &data_dir_str) {
    std::filesystem::path index_dir = index_dir_str;
    std::filesystem::path offsets_dir = index_dir_str + "/offsets";
    std::filesystem::path fst_dir = index_dir_str + "/fsts";
    std::filesystem::path data_dir = data_dir_str;

    // Create index directories, fails gracefully if directory already exists
    std::filesystem::create_directory(index_dir);
    std::filesystem::create_directory(offsets_dir);
    std::filesystem::create_directory(fst_dir);

    // Check data directory exists
    if (!std::filesystem::exists(data_dir)) {
        throw std::invalid_argument("Data directory does not exist");
    }

    // Get list of supported files to index
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(data_dir)) {
        if (entry.is_regular_file() && valid_extension(entry.path().extension())) {
            files.push_back(entry.path());
        }
    }

    std::set<std::string> file_mappings;
    std::vector<std::string> file_mappings_vec;
    // Tokenize, generate offsets and fst's for all supported files
    for (auto &f : files) {
        file_mappings.insert(f.filename().string());
    }

    size_t i = 0;
    for (auto &f : file_mappings) {
        file_mappings_vec.push_back(f);

        Tokenizer tokenizer = Tokenizer::from_file(data_dir_str + '/' + f);
        //tokenizer.printStatistics();

        std::string file_id = std::to_string(i);
        generate_offsets_file(tokenizer, offsets_dir.string() + '/' + file_id);

        //std::cout << "Creating Alpha FST" << std::endl;

        create_fst<std::string>(tokens_from_map<std::string>(tokenizer.alpha_tokens), fst_dir.string() + "/" + file_id + "_alpha.fst");
        //check_fst(tokenizer.getAlphaTokens(), fst_dir.string() + "/" + file_id + "_alpha.fst");
        tokenizer.alpha_tokens.clear();
        //std::cout << "Finished" << std::endl;

        //std::cout << "Creating Int FST" << std::endl;
        create_fst<std::vector<uint8_t>>(tokens_from_map<std::vector<uint8_t>>(tokenizer.num_tokens),  fst_dir.string() + "/" + file_id + "_int.fst");
        //check_fst(tokenizer.getNumTokens(), fst_dir.string() + "/" + file_id + "_int.fst");
        tokenizer.num_tokens.clear();
        //std::cout << "Finished" << std::endl;

        //std::cout << "Creating Float FST" << std::endl;
        create_fst<std::vector<uint8_t>>(tokens_from_map<std::vector<uint8_t>>(tokenizer.float_tokens),  fst_dir.string() + "/" + file_id + "_float.fst");
        //check_fst(tokenizer.getFloatTokens(), fst_dir.string() + "/" + file_id + "_float.fst");
        tokenizer.float_tokens.clear();
       //std::cout << "Finished" << std::endl;

        //std::cout << "Creating Date FST" << std::endl;
        create_fst<std::vector<uint8_t>>(tokens_from_map<std::vector<uint8_t>>(tokenizer.d_tokens),  fst_dir.string() + "/" + file_id + "_date.fst");
        //check_fst(tokenizer.getDateTokens(), fst_dir.string() + "/" + file_id + "_date.fst");
        tokenizer.d_tokens.clear();
        //std::cout << "Finished" << std::endl;

        //std::cout << "Creating Time FST" << std::endl;
        create_fst<std::vector<uint8_t>>(tokens_from_map<std::vector<uint8_t>>(tokenizer.t_tokens),  fst_dir.string() + "/" + file_id + "_time.fst");
        //check_fst(tokenizer.getTimeTokens(), fst_dir.string() + "/" + file_id + "_time.fst");
        tokenizer.t_tokens.clear();
        //std::cout << "Finished" << std::endl;

        //std::cout << "Creating IPV4 FST" << std::endl;
        create_fst<std::vector<uint8_t>>(tokens_from_map<std::vector<uint8_t>>(tokenizer.ipv4_tokens), fst_dir.string() + "/" + file_id + "_ipv4.fst");
        //check_fst(tokenizer.getIPTokens(), fst_dir.string() + "/" + file_id + "_ipv4.fst");
        tokenizer.ipv4_tokens.clear();
        //std::cout << "Finished" << std::endl;

        ++i;
    }

    create_fst<std::string>(file_mappings_vec, index_dir.string() + "/" + "file_mappings.fst");
    //check_fst<std::string>(file_mappings_vec, index_dir.string() + "/" + "file_mappings.fst");
}

const std::vector<uint64_t> get_id_offsets_from_file(const std::string &file_name, uint8_t type) {
    MappedFile mf = open_mmap_r(file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());

    size_t file_size = mf.size();

    
    // Read in header
    Offsets_header header = {};
    std::memcpy(reinterpret_cast<char*>(&header), &file[0], sizeof(header));

    size_t size_to_read;
    size_t addr_offset;

    // Should only ever be requesting id_offsets if present?
    switch (type) {
        case TokenType::ALPHA:
            size_to_read = header.alpha_offsets_header - header.alpha_id_map_header;
            if (size_to_read == 0) throw std::invalid_argument("No alpha ID's present in file");
            addr_offset = header.alpha_id_map_header;
            break;
        case TokenType::INT:
            size_to_read = header.num_offsets_header - header.num_id_map_header;
            if (size_to_read == 0) throw std::invalid_argument("No int ID's present in file");
            addr_offset = header.num_id_map_header;
            break;
        case TokenType::FLOAT:
            size_to_read = header.float_offsets_header - header.float_id_map_header;
            if (size_to_read == 0) throw std::invalid_argument("No float ID's present in file");
            addr_offset = header.float_id_map_header;
            break;
        case TokenType::DATE:
            size_to_read = header.date_offsets_header - header.date_id_map_header;
            if (size_to_read == 0) throw std::invalid_argument("No date ID's present in file");
            addr_offset = header.date_id_map_header;
            break;
        case TokenType::TIME:
            size_to_read = header.time_offsets_header - header.time_id_map_header;
            if (size_to_read == 0) throw std::invalid_argument("No time ID's present in file");
            addr_offset = header.time_id_map_header;
            break;
        case TokenType::IPV4:
            size_to_read = header.ipv4_offsets_header - header.ipv4_id_map_header;
            if (size_to_read == 0) throw std::invalid_argument("No ipv4 ID's present in file");
            addr_offset = header.ipv4_id_map_header;
            break;
        default:
            break;
    }

    std::vector<uint8_t> enc_id_to_offset(size_to_read);
    std::memcpy(reinterpret_cast<char*>(enc_id_to_offset.data()), &file[addr_offset], size_to_read);

    std::vector<uint64_t> id_to_offset = gvb_decode_uint64_t(enc_id_to_offset);

    return id_to_offset;
}

// Output converts offsets values from file from 1 indexing back to 0 indexing
const std::vector<uint64_t> get_tok_offsets_from_file(const uint64_t &token_id, const std::vector<uint64_t> &id_to_offset, const std::string &file_name, uint8_t type) {
    MappedFile mf = open_mmap_r(file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    size_t file_size = mf.size();

    // Read in header
    Offsets_header header = {};
    std::memcpy(reinterpret_cast<char*>(&header), &file[0], sizeof(header));

    // Get offset to start of token offsets in file
    uint64_t size_to_read;
    uint64_t tok_offsets_length;
    uint64_t offset;

    switch (type) {
        case TokenType::ALPHA:
            tok_offsets_length = header.num_id_map_header - header.alpha_offsets_header;
            if (tok_offsets_length == 0) throw std::invalid_argument("No alpha offsets present in file");
            offset = header.alpha_offsets_header;
            break;
        case TokenType::INT:
            tok_offsets_length = header.float_id_map_header - header.num_offsets_header;
            if (tok_offsets_length == 0) throw std::invalid_argument("No int offsets present in file");
            offset = header.num_offsets_header;
            break;
        case TokenType::FLOAT:
            tok_offsets_length = header.date_id_map_header - header.float_offsets_header;
            if (tok_offsets_length == 0) throw std::invalid_argument("No float offsets present in file");
            offset = header.float_offsets_header;
            break;
        case TokenType::DATE:
            tok_offsets_length = header.time_id_map_header - header.date_offsets_header;
            if (tok_offsets_length == 0) throw std::invalid_argument("No date offsets present in file");
            offset = header.date_offsets_header;
            break;
        case TokenType::TIME:
            tok_offsets_length = header.ipv4_id_map_header - header.time_offsets_header;
            if (tok_offsets_length == 0) throw std::invalid_argument("No time offsets present in file");
            offset = header.time_offsets_header;
            break;
        case TokenType::IPV4:
            tok_offsets_length = header.entry_offsets_header - header.ipv4_offsets_header; 
            if (tok_offsets_length == 0) throw std::invalid_argument("No ipv4 offsets present in file");
            offset = header.ipv4_offsets_header;
            break;
        default:
            break;
    }

    if (token_id == id_to_offset.size() - 1) {
        size_to_read = tok_offsets_length - id_to_offset[token_id] + 1;
    } else {
        size_to_read = id_to_offset[token_id+1] - id_to_offset[token_id];
    }

    std::vector<uint8_t> enc_tok_offsets(size_to_read);
    std::memcpy(reinterpret_cast<char*>(enc_tok_offsets.data()), &file[offset + id_to_offset[token_id]-1], size_to_read);

    std::vector<uint64_t> tok_offsets = gvb_decode_uint64_t(enc_tok_offsets);
    std::vector<uint64_t> offsets = delta_decode(tok_offsets);

    // Convert back to 0 indexing
    for (auto &o : offsets) --o;

    return offsets;
}

// Output converts offsets values from file from 1 indexing back to 0 indexing
const std::vector<uint64_t> get_entry_offsets_from_file(const std::string &file_name) {
    MappedFile mf = open_mmap_r(file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    size_t file_size = mf.size();

    // Read in header
    Offsets_header header = {};
    std::memcpy(reinterpret_cast<char*>(&header), &file[0], sizeof(header));

    uint64_t size_to_read = file_size - header.entry_offsets_header;

    if (size_to_read == 0) throw std::invalid_argument("No entry offsets present in file");

    std::vector<uint8_t> enc_entry_offsets_vec(size_to_read);
    std::memcpy(reinterpret_cast<char*>(enc_entry_offsets_vec.data()), &(file[header.entry_offsets_header]), sizeof(uint8_t) * size_to_read);

    std::vector<uint64_t> entry_offsets_vec = gvb_decode_uint64_t(enc_entry_offsets_vec);

    assert(entry_offsets_vec.size() % 2 == 0);

    // Convert to 0 indexing
    for (auto &cur : entry_offsets_vec) {
        --cur;
    }

    return entry_offsets_vec;
}

const std::unordered_map<size_t, std::vector<uint64_t>> search_index(const std::string &query) {
    std::unordered_map<size_t, std::vector<uint64_t>> offsets;

    Tokenizer tokens = Tokenizer::from_string(query);

    //tokens.printStatistics();

    // Check index/fsts directory exists
    if (!std::filesystem::exists("index/fsts")) { // TODO: HARDCODED, NEED TO CHANGE
        throw std::invalid_argument("Fst's directory does not exist");
    }

    // Check index/fsts directory exists
    if (!std::filesystem::exists("index/offsets")) { // TODO: HARDCODED, NEED TO CHANGE
        throw std::invalid_argument("Offset's directory does not exist");
    }

    // Check file_mappings file exists
    if (!std::filesystem::exists("index/file_mappings.fst")) { // TODO: HARDCODED, NEED TO CHANGE
        throw std::invalid_argument("file_mappings file does not exist");
    }

    // Read in file_mappings fst
    fst::StdConstFst *fst = nullptr;
    read_from_disk(fst, "index/file_mappings.fst");
    std::set<std::string> names = get_strings_from_fst<std::string>(*fst);
    size_t names_size = names.size();

    for (auto &t : tokens.alpha_tokens) {
        search_index_by_type(offsets, t.first, TokenType::ALPHA, names_size);
    }

    for (auto &t : tokens.num_tokens) {
        search_index_by_type(offsets, to_bytes(t.first), TokenType::INT, names_size);
    }

    for (auto &t : tokens.float_tokens) {
        search_index_by_type(offsets, to_bytes(t.first), TokenType::FLOAT, names_size);
    }

    for (auto &t : tokens.d_tokens) {
        search_index_by_type(offsets, to_bytes(t.first), TokenType::DATE, names_size);
    }

    for (auto &t : tokens.t_tokens) {
        search_index_by_type(offsets, to_bytes(t.first), TokenType::TIME, names_size);
    }
    for (auto &t : tokens.ipv4_tokens) {
        search_index_by_type(offsets, to_bytes(t.first), TokenType::IPV4, names_size);
    }

    return offsets;
}

const uint8_t str_to_token_type(const std::string &type_str) {
    if (type_str == "alpha") return TokenType::ALPHA;
    if (type_str == "int") return TokenType::INT;
    if (type_str == "float") return TokenType::FLOAT;
    if (type_str == "date") return TokenType::DATE;
    if (type_str == "time") return TokenType::TIME;
    if (type_str == "ipv4") return TokenType::IPV4;

    return TokenType::INVALID;
}

const std::string token_type_to_str(const uint8_t &type) {
    switch (type) {
        case TokenType::ALPHA:
            return "alpha";
        case TokenType::INT:
            return "int";
        case TokenType::FLOAT:
            return "float";
        case TokenType::DATE:
            return "date";
        case TokenType::TIME:
            return "time";
        case TokenType::IPV4:
            return "ipv4";
        default:
            throw std::invalid_argument("Type not valid");
    }
}

/*  Offset File Layout
    offset_headers - File offset for position of the following values
    alpha_offsets
    alpha_id_to_offsetsa
    num_offsets
    num_id_to_offsets
    float_offsets
    float_id_to_offsets
    date_offsets
    date_id_to_offsets
    time_offsets
    time_id_to_offsets
    ipv4_offsets
    ip4v_id_to_offsets
*/ 
void generate_offsets_file(const Tokenizer &tokenizer, const std::string &file_name) {
    std::ofstream file;
    file.open(file_name + ".off", std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_name);
    }

    Offsets_header header = {};
    file.write(reinterpret_cast<const char*>(&header), sizeof(header)); // Reserve for later use
    
    const Write_pos alpha_pos = write_offsets_to_file(tokenizer.alpha_tokens, file);
    const Write_pos num_pos = write_offsets_to_file(tokenizer.num_tokens, file);
    const Write_pos float_pos = write_offsets_to_file(tokenizer.float_tokens, file);
    const Write_pos date_pos = write_offsets_to_file(tokenizer.d_tokens, file);
    const Write_pos time_pos = write_offsets_to_file(tokenizer.t_tokens, file);
    const Write_pos ipv4_pos = write_offsets_to_file(tokenizer.ipv4_tokens, file);
    const uint64_t entry_offsets_pos = write_entry_offsets_to_file(tokenizer.entry_offsets, file); // Handled differently to above



    // Populate header with offset values then write to file   
    header = {
        alpha_pos.id_map_pos, alpha_pos.offsets_pos,
        num_pos.id_map_pos, num_pos.offsets_pos,
        float_pos.id_map_pos, float_pos.offsets_pos,
        date_pos.id_map_pos, date_pos.offsets_pos,
        time_pos.id_map_pos, time_pos.offsets_pos,
        ipv4_pos.id_map_pos, ipv4_pos.offsets_pos,
        entry_offsets_pos
    };

    file.seekp(0); 
    file.write(reinterpret_cast<const char*>(&header), sizeof(header)); // Write header data

    file.close();

    //check_offsets_file(tokenizer, file_name + ".off");
}

const uint64_t write_entry_offsets_to_file(const std::vector<std::pair<uint64_t, uint64_t>> &entry_offsets, std::ofstream &file) {
    if (entry_offsets.size() == 0) { 
        return static_cast<uint64_t>(file.tellp());
    }

    // Convert into a standard vector
    std::vector<uint64_t> entry_offsets_vec;
    for (auto &cur : entry_offsets) {
        entry_offsets_vec.push_back(cur.first);
        entry_offsets_vec.push_back(cur.second);
    }
    
    std::vector<uint8_t> all_enc_entry_offsets = gvb_encode_uint64_t(entry_offsets_vec);

    // Write to file
    const uint64_t entry_offsets_size = all_enc_entry_offsets.size();

    const uint64_t write_pos = file.tellp();
    file.write(reinterpret_cast<const char*>(all_enc_entry_offsets.data()), sizeof(uint8_t) * entry_offsets_size);

    return write_pos;
}

void check_offsets_file(const Tokenizer &tokenizer, const std::string &file_name) {
    bool check = true;
    check = check && check_offsets_file_by_type(tokenizer.alpha_tokens, file_name, TokenType::ALPHA);
    check = check && check_offsets_file_by_type(tokenizer.num_tokens, file_name, TokenType::INT);
    check = check && check_offsets_file_by_type(tokenizer.float_tokens, file_name, TokenType::FLOAT);
    check = check && check_offsets_file_by_type(tokenizer.d_tokens, file_name, TokenType::DATE);
    check = check && check_offsets_file_by_type(tokenizer.t_tokens, file_name, TokenType::TIME);
    check = check && check_offsets_file_by_type(tokenizer.ipv4_tokens, file_name, TokenType::IPV4);
    check = check && check_entry_offsets(tokenizer.entry_offsets, file_name);

    if (check) {
        std::cout << file_name << ": Offsets file correct" << std::endl;
    } else {
        std::cout << file_name << ": Offsets file incorrect" << std::endl;
    }
}

bool check_entry_offsets(std::vector<std::pair<uint64_t, uint64_t>> entry_offsets, const std::string &file_name) {
    // Convert into a standard vector and into 0 indexing
    std::vector<uint64_t> entry_offsets_vec;
    for (auto &cur : entry_offsets) {
        entry_offsets_vec.push_back(cur.first - 1);
        entry_offsets_vec.push_back(cur.second - 1);
    }

    std::vector<uint64_t> input = get_entry_offsets_from_file(file_name);

    if (input == entry_offsets_vec) {
        //std::cout << "Entry offsets correct: " << std::endl;
        return true;
    } else {
       std::cout << "Entry offsets incorrect: " << std::endl;
       return false;
    }
}

const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> get_entries_offsets(const std::unordered_map<size_t, std::vector<uint64_t>> &offsets_map, const std::vector<std::string> &file_names, const std::string &data_path, const std::string &index_path) {
    std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> line_offsets;
    // Offsets is a map of file_id's to offsets
    for (auto &o : offsets_map) {
        const uint64_t file_id = o.first;
        const std::string data_file_path = data_path + '/' + file_names[file_id];
        const std::string index_file_path = index_path + "/offsets/" + std::to_string(file_id)+ ".off";
        const std::vector<uint64_t> &token_offsets = o.second;

        // All entry offsets for current data file:
        std::vector<uint64_t> entry_offsets = get_entry_offsets_from_file(index_file_path);
        std::sort(entry_offsets.begin(), entry_offsets.end());

        // For each token_offset, get closest prev newline and match in file to get offset to end of line
        for (auto &cur : token_offsets) {

            // Binary search for start, then get the next value (always the respective end value)
            // Faster than using unordered_map, at least for smaller datasets
            auto it_1 = std::lower_bound(entry_offsets.begin(), entry_offsets.end(), cur);
            if (it_1 == entry_offsets.end() || *it_1 != cur) {
                --it_1;
            }

            line_offsets[file_id][*it_1] = *(it_1+1);
        }
        
    }

    return line_offsets;
}

const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> get_entries_offsets_intersection(const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> &entries_1, const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> &entries_2) {
    std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> intersection_offsets;
    // Offsets is a map of file_id's to offsets
    for (auto &o : entries_1) {
        const size_t file_id = o.first;
        if (!entries_2.contains(file_id)) continue;

        const std::unordered_map<uint64_t, uint64_t> &entry_offsets_1 = o.second;
        const std::unordered_map<uint64_t, uint64_t> &entry_offsets_2 = entries_2.at(file_id);
        for (auto &entry : entry_offsets_1) {
            // Entry present in both maps
            if (entry_offsets_2.contains(entry.first)) {
                intersection_offsets[file_id][entry.first] = entry.second;
            }
        }

    }

    return intersection_offsets;
}

char* get_entry_str(const uint64_t &line_start, const uint64_t &line_end, const char *file) {
    const uint64_t size_to_read = line_end - line_start;

    char* str = static_cast<char*>(malloc(size_to_read + 1));

    std::memcpy(str, &file[line_start], size_to_read);

    str[size_to_read] = 0;

    return str;
}

const uint64_t find_line_start(const uint64_t &offset, std::ifstream &file) {
    const uint64_t BUF_SIZE = 100;
    std::vector<uint8_t> buf(BUF_SIZE);

    uint64_t chunk_size, start; // Prevent repeated initialisation in the loop
    uint64_t end = offset;
    while (end > 0) {
        chunk_size = std::min<uint64_t>(BUF_SIZE, end);
        start = end - chunk_size;

        file.seekg(start);
        file.read(reinterpret_cast<char*>(buf.data()), chunk_size);

        for (uint64_t j = chunk_size; j-- > 0; ) {
            if (buf[j] == '\n') {
                return start + j + 1; // Move forward past newline
            }
        }

        end = start;
    }

    return 0; // Start of file
}

const uint64_t find_line_start_1(const uint64_t &offset, std::ifstream &file) {
    uint64_t cur_offset = offset;

    char c;
    while (cur_offset != 0) {
        file.seekg(cur_offset);

        file.get(c);

        if (c == '\n')  {
            ++cur_offset; // Move forward past newline
            break;
        }

        --cur_offset;
    }

    return cur_offset; // Start of file
}

const uint64_t forward_entry_offset(const uint64_t &offset, std::ifstream &file) {
    uint64_t cur_offset = offset;

    file.seekg(cur_offset);

    char c;
    while (true) {
        if (!file.get(c)) break;
        if (c == '\n') break;

        ++cur_offset;
    }

    return cur_offset;
}

void print_entries(const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> &map, const std::vector<std::string> &names_vec, std::string data_path) {
    for (auto &i : map) {
        std::cout << "File: " << names_vec[i.first] << std::endl;
        const uint64_t file_id = i.first;
        const std::string file_path = data_path + '/' + names_vec[file_id];

        MappedFile mf = open_mmap_r(file_path);
        const char *file = static_cast<char*>(mf.addr());
        size_t file_size = mf.size();

        for (auto &line : i.second) {
            char* str = get_entry_str(line.first, line.second, file);
            std::cout << "Line: " <<  str << std::endl;
            free(str);
        }
    }
}