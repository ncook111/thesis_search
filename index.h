#ifndef INDEX_H
#define INDEX_H

#include <vector>
#include <fstream>
#include <map>
#include "coder.h"
#include "tokenizer.h"
#include "fst.h"
#include "helper.h"
#include "custom_types.h"

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> EntryMap;

bool valid_extension(const std::string &ext);
void create_index(const std::string &index_dir_str, const std::string &data_dir_str);
const std::vector<uint64_t> get_id_offsets_from_file(const std::string &file_name, uint8_t type);
const std::vector<uint64_t> get_tok_offsets_from_file(const uint64_t &token_id, const std::vector<uint64_t> &id_to_offset, const std::string &file_name, uint8_t type);
void generate_offsets_file(const Tokenizer &tokenizer, const std::string &file_name);
const FieldMap search_index(const std::string &query, const std::string &index_path);
void check_offsets_file(const Tokenizer &tokenizer, const std::string &file_name);
const uint8_t str_to_token_type(const std::string &type_str);
const std::string token_type_to_str(const uint8_t &type);
char* get_entry_str(const uint64_t &line_start, const uint64_t &line_end, const char *file);
const EntryMap get_entries_offsets_intersection(const EntryMap &entries_1, const EntryMap &entries_2);
const uint64_t write_entry_offsets_to_file(const std::vector<uint64_t> &entry_offsets, std::ofstream &file);
const std::vector<uint64_t> get_entry_offsets_from_file(const std::string &file_name);
bool check_entry_offsets(std::vector<uint64_t> entry_offsets, const std::string &file_name);
const EntryMap get_entries_offsets(const FieldMap &offsets_map, const std::vector<std::string> &file_names, const std::string &data_path, const std::string &index_path);
void print_entries(const EntryMap &map, const std::string &index_path, const std::string &data_path);
const EntryMap process_queries(const std::vector<std::string> &queries, const std::string &data_path, const std::string &index_path);

// Saved to the front of the file
struct Offsets_header {
    uint64_t alpha_id_map_header;
    uint64_t alpha_offsets_header;
    uint64_t num_id_map_header;
    uint64_t num_offsets_header;
    uint64_t float_id_map_header;
    uint64_t float_offsets_header;
    uint64_t date_id_map_header;
    uint64_t date_offsets_header;
    uint64_t time_id_map_header;
    uint64_t time_offsets_header;
    uint64_t ipv4_id_map_header;
    uint64_t ipv4_offsets_header;
    uint64_t entry_offsets_header;
};

struct Write_pos {
  uint64_t id_map_pos;
  uint64_t offsets_pos;
};

template <QueryType Q>
void search_index_by_type(FieldMap &offsets, const Q &query, const uint8_t type, size_t files_indexed) {
    std::string type_str = token_type_to_str(type);
    // Get list of all fst files filtered by type
    size_t i = 0;
    for (; i < files_indexed; ++i) {
        std::string fst_file_name = "index/fsts/" + std::to_string(i) + '_' + type_str + ".fst";
        std::string offset_file_name = "index/offsets/" + std::to_string(i) + ".off";
        if (!std::filesystem::exists(fst_file_name)) { 
            throw std::invalid_argument("Fst file does not exist");
        }

        fst::StdConstFst *fst = nullptr;
        read_from_disk(fst, fst_file_name);

        int id = get_id_from_fst(*fst, query);
        if (id == -1) continue; // Not found

        std::vector<uint64_t> id_offsets_map = get_id_offsets_from_file(offset_file_name, type);
        std::vector<uint64_t> tokens = get_tok_offsets_from_file(id, id_offsets_map, offset_file_name, type);

        offsets[i] = std::move(tokens);
    }
}

template <MapType M>
const Write_pos write_offsets_to_file(const std::map<M, std::vector<uint64_t>> &tokens_map, std::ofstream &file) {
    if (tokens_map.size() == 0) { 
        return Write_pos {
            static_cast<uint64_t>(file.tellp()),
            static_cast<uint64_t>(file.tellp())
        };
    }

    std::vector<uint8_t> all_enc_tok_offsets;
    std::vector<uint64_t> id_to_offset;

    uint64_t offset = 1; // Offsets begin at 1 as a requirement of the gvb encoding and decoding steps
                         // A value of 0 is used as terminator when decoding
    uint64_t count = 0;
    for (auto it = tokens_map.begin(); it != tokens_map.end(); ++it) {
        const std::vector<uint64_t> &offsets = it->second;
        count += offsets.size();
        std::vector<uint8_t> enc_tok_offsets = gvb_encode_uint64_t(delta_encode(offsets));
        std::move(enc_tok_offsets.begin(), enc_tok_offsets.end(), back_inserter(all_enc_tok_offsets));
        id_to_offset.push_back(offset);

        offset += enc_tok_offsets.size();
    }

    const std::vector<uint8_t> enc_id_to_offset = gvb_encode_uint64_t(id_to_offset);

    // Write to file
    const uint64_t id_to_offset_size = enc_id_to_offset.size();
    const uint64_t tok_offsets_size = all_enc_tok_offsets.size();

    const uint64_t first_write_pos = file.tellp();
    file.write(reinterpret_cast<const char*>(enc_id_to_offset.data()), sizeof(uint8_t) * id_to_offset_size);

    const uint64_t second_write_pos = file.tellp();
    file.write(reinterpret_cast<const char*>(all_enc_tok_offsets.data()), sizeof(uint8_t) * tok_offsets_size);

    return Write_pos {first_write_pos, second_write_pos};
}

template <MapType M>
bool check_offsets_file_by_type(const std::map<M, std::vector<uint64_t>> &tokens_map, const std::string &file_name, uint8_t type) {
    std::vector<uint64_t> id_to_offset;
    uint64_t offset = 1;
    for (auto it = tokens_map.begin(); it != tokens_map.end(); ++it) {
        const std::vector<uint64_t> &offsets = it->second;
        const std::vector<uint8_t> enc_tok_offsets = gvb_encode_uint64_t(delta_encode(offsets));
        id_to_offset.push_back(offset);
        offset += enc_tok_offsets.size();
    }

    bool empty = id_to_offset.size() == 0;
    bool except = false;

    std::vector<uint64_t> input;
    
    try {
        input = get_id_offsets_from_file(file_name, type);
    } catch (const std::invalid_argument &e) {
        except = true;
    }

    std::string type_string = token_type_to_str(type);

    bool ret;
    if (input == id_to_offset || (empty && except)) {
        ret = true;
        //std::cout << type_string << " ID offsets correct: " << file_name << std::endl;
    } else {
        ret = false;
        std::cout << type_string << " ID offsets incorrect: " << file_name << std::endl;
    }

    offset = 1;
    uint64_t id = 0;
    empty = false;
    except = false;
    bool correct = true;
    for (auto it = tokens_map.begin(); it != tokens_map.end(); ++it) {
        std::vector<uint64_t> offsets;
        
        // Convert offsets to 0 indexing
        for (auto &i : it->second) {
            offsets.push_back(i - 1);
        }

        std::vector<uint64_t> from_file;
        try {
             from_file = get_tok_offsets_from_file(id, id_to_offset, file_name, type);
        } catch (const std::invalid_argument &e) {
            except = true;
        }
         
        correct = from_file == offsets;

        ++id;
    }

    if (correct || (empty && except)) {
        //std::cout << type_string << " Token offsets correct: " << file_name << std::endl;
        return ret && true;
    } else {
        std::cout << type_string << " Token offsets incorrect: " << file_name << std::endl;
        
       return false;
    }
}

#endif