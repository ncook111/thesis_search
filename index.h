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
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;

bool valid_extension(const std::string &ext);
void create_index(const std::string &index_dir_str, const std::string &data_dir_str, const uint8_t enc_type);
const std::vector<uint64_t> get_id_offsets_from_file(const std::string &file_name, const uint8_t type, const uint8_t enc_type);
const std::vector<uint64_t> get_tok_offsets_from_file(const uint64_t &token_id, const std::vector<uint64_t> &id_to_offset, const std::string &file_name, uint8_t type, const uint8_t enc_type);
void generate_offsets_file(const Tokenizer &tokenizer, const std::string &file_name, const uint8_t enc_type);
const FieldMap search_index(const std::string &query, const std::string &index_path, const uint8_t enc_type);
void check_offsets_file(const Tokenizer &tokenizer, const std::string &file_name, const uint8_t enc_type);
const uint8_t str_to_token_type(const std::string &type_str);
const std::string token_type_to_str(const uint8_t &type);
char* get_entry_str(const uint64_t &line_start, const uint64_t &line_end, const char *file);
const FieldMap get_entries_offsets_intersection(const FieldMap &entries_1, const FieldMap &entries_2);
const uint64_t write_entry_offsets_to_file(const std::vector<uint64_t> &entry_offsets, std::ofstream &file, const uint8_t enc_type);
const std::vector<uint64_t> get_entry_offsets_from_file(const std::string &file_name);
bool check_entry_offsets(std::vector<uint64_t> entry_offsets, const std::string &file_name, const uint8_t enc_type);
const EntryMap get_entries_offsets(const FieldMap &offsets_map, const std::vector<std::string> &file_names, const std::string &data_path, const std::string &index_path);
void print_entries(const EntryMap &map, const std::string &index_path, const std::string &data_path);
const EntryMap process_queries(const std::vector<std::string> &queries, const std::string &data_path, const std::string &index_path, const uint8_t enc_type);
const uint64_t write_entry_gvb(const std::vector<uint64_t> &entry_offsets, std::ofstream &file);
const uint64_t write_entry_pef(const std::vector<uint64_t> &entry_offsets, std::ofstream &file);
const std::vector<uint64_t> read_id_offsets_gvb(const uint8_t *file, const uint64_t &addr_offset, const uint64_t &size_to_read);
const std::vector<uint64_t> read_id_offsets_pef(const uint8_t *file, const uint64_t &addr_offset, const uint64_t &size_to_read);
const std::vector<uint64_t> read_tok_offsets_gvb(const uint8_t *file, const uint64_t &addr_offset, const uint64_t &size_to_read); 
const std::vector<uint64_t> read_tok_offsets_pef(const uint8_t *file, const uint64_t &addr_offset, const uint64_t &size_to_read);

// Saved to the front of the file
struct Offsets_header {
    uint64_t alpha_id_map_header, alpha_offsets_header;
    uint64_t num_id_map_header, num_offsets_header;
    uint64_t float_id_map_header, float_offsets_header;
    uint64_t date_id_map_header, date_offsets_header;
    uint64_t time_id_map_header, time_offsets_header;
    uint64_t ipv4_id_map_header, ipv4_offsets_header;
    uint64_t entry_offsets_header;
};

struct Pef_Header {
    uint64_t num_bits;
    uint64_t universe;
    uint64_t n;
};

struct Write_pos {
  uint64_t id_map_pos;
  uint64_t offsets_pos;
};

template <QueryType Q>
void search_index_by_type(FieldMap &offsets, const Q &query, const uint8_t type, size_t files_indexed, const std::string &index_path, const uint8_t enc_type) {
    std::string type_str = token_type_to_str(type);
    // Get list of all fst files filtered by type
    size_t i = 0;
    for (; i < files_indexed; ++i) {
        std::string fst_file_name = index_path + "/fsts/" + std::to_string(i) + '_' + type_str + ".fst";
        std::string offset_file_name = index_path + "/offsets/" + std::to_string(i) + ".off";
        if (!std::filesystem::exists(fst_file_name)) { 
            std::cout << fst_file_name << std::endl;
            throw std::invalid_argument("Fst file does not exist");
        }

        fst::StdConstFst *fst = nullptr;
        read_from_disk(fst, fst_file_name);

        int id = get_id_from_fst(*fst, query);
        if (id == -1) continue; // Not found

        std::vector<uint64_t> id_offsets_map = get_id_offsets_from_file(offset_file_name, type, enc_type);
        std::vector<uint64_t> tokens = get_tok_offsets_from_file(id, id_offsets_map, offset_file_name, type, enc_type);

        offsets[i] = std::move(tokens);
    }
}

template <MapType M>
const Write_pos write_offsets_gvb(const std::map<M, std::vector<uint64_t>> &tokens_map, std::ofstream &file) {
    std::vector<uint8_t> all_enc_tok_offsets;
    std::vector<uint64_t> id_to_offset;

    uint64_t offset = 1; // Offsets begin at 1 as a requirement of the gvb encoding and decoding steps
                         // A value of 0 is used as a terminator when decoding in under special circumstances
    for (auto it = tokens_map.begin(); it != tokens_map.end(); ++it) {
        // Duplicate docids may be present from tokenization stage, remove them
        const std::vector<uint64_t> &offsets = it->second;
        std::vector<uint64_t> offsets_dedup;
        for (auto &off : offsets) {
            if (offsets_dedup.size() && offsets_dedup.back() == off) continue;
            offsets_dedup.push_back(off);
            assert(off != 0);
        }

        assert(offsets_dedup.size() != 0);

        std::vector<uint8_t> enc_tok_offsets = gvb_encode_uint64_t(delta_encode(offsets_dedup));
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
const Write_pos write_offsets_pef(const std::map<M, std::vector<uint64_t>> &tokens_map, std::ofstream &file) {
    std::vector<std::pair<Pef_Header, std::unique_ptr<succinct::bit_vector>>> all_enc_tok_offsets;
    std::vector<uint64_t> id_to_offset;

    uint64_t offset = 1; // Offsets begin at 1 as a requirement of the gvb encoding and decoding steps
                         // A value of 0 is used as terminator when decoding
    for (auto it = tokens_map.begin(); it != tokens_map.end(); ++it) {
        // Duplicate docids may be present from tokenization stage, remove them
        const std::vector<uint64_t> &offsets = it->second;
        std::vector<uint64_t> offsets_dedup;
        for (auto &off : offsets) {
            if (offsets_dedup.size() && offsets_dedup.back() == off) continue;
            offsets_dedup.push_back(off);
        }


        succinct::bit_vector_builder enc_tok_offsets;
        const std::pair<uint64_t, uint64_t> res = partitioned_ef_encode(enc_tok_offsets, offsets_dedup);
        const uint64_t num_bits = enc_tok_offsets.size();

        const Pef_Header header = {num_bits, res.first, res.second};

        all_enc_tok_offsets.push_back(std::make_pair(header, std::make_unique<succinct::bit_vector>(&enc_tok_offsets)));

        id_to_offset.push_back(offset);

        offset += sizeof(header) + all_enc_tok_offsets.back().second->data().size() * sizeof(uint64_t);
    }

    succinct::bit_vector_builder enc_id_to_offset;
    const std::pair<uint64_t, uint64_t> res = partitioned_ef_encode(enc_id_to_offset, id_to_offset);
    const uint64_t num_bits = enc_id_to_offset.size();
    const Pef_Header header = {num_bits, res.first, res.second};
    succinct::bit_vector final_enc_id_to_offset = &enc_id_to_offset;

    // Write to file
    const uint64_t first_write_pos = file.tellp();
    
    // Write num_bits, universe and n
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write id_to_offsets data
    uint64_t size = final_enc_id_to_offset.data().size() * sizeof(uint64_t);
    file.write(reinterpret_cast<const char*>(final_enc_id_to_offset.data().data()), size);


    const uint64_t second_write_pos = file.tellp();

    for (auto &cur : all_enc_tok_offsets) {
        const Pef_Header &cur_header = cur.first;
        succinct::bit_vector &bv = *cur.second;
        // Write num_bits, universe and n
        file.write(reinterpret_cast<const char*>(&cur_header), sizeof(cur_header));
        
        // Write tok_offsets data
        size = bv.data().size() * sizeof(uint64_t);
        file.write(reinterpret_cast<const char*>(bv.data().data()), size);
    }

    return Write_pos {first_write_pos, second_write_pos};
}

template <MapType M>
const Write_pos write_offsets_to_file(const std::map<M, std::vector<uint64_t>> &tokens_map, std::ofstream &file, const uint8_t enc_type) {
    if (tokens_map.size() == 0) { 
        return Write_pos {
            static_cast<uint64_t>(file.tellp()),
            static_cast<uint64_t>(file.tellp())
        };
    }

    switch (enc_type) {
        case EncType::GVB:
            return write_offsets_gvb(tokens_map, file);
            break;
        case EncType::PEF:
            return write_offsets_pef(tokens_map, file);
            break;
        default:
            throw std::invalid_argument("Not a valid encoding type");
    }


}

template <MapType M>
bool check_offsets_file_by_type(const std::map<M, std::vector<uint64_t>> &tokens_map, const std::string &file_name, uint8_t type, const uint8_t enc_type) {
    std::vector<uint64_t> id_to_offset;
    uint64_t offset = 1;

    if (enc_type == EncType::GVB) {
        for (auto it = tokens_map.begin(); it != tokens_map.end(); ++it) {
            std::vector<uint64_t> offsets;
            for (auto &i : it->second) {
                if (offsets.size() && offsets.back() == i) continue;
                offsets.push_back(i);
            }
            const std::vector<uint8_t> enc_tok_offsets = gvb_encode_uint64_t(delta_encode(offsets));
            id_to_offset.push_back(offset);
            offset += enc_tok_offsets.size();
        }
    } else if (enc_type == EncType::PEF) {
        for (auto it = tokens_map.begin(); it != tokens_map.end(); ++it) {
            // Duplicate docids may be present from tokenization stage, remove them
            const std::vector<uint64_t> &offsets = it->second;
            std::vector<uint64_t> offsets_dedup;
            for (auto &off : offsets) {
                if (offsets_dedup.size() && offsets_dedup.back() == off) continue;
                offsets_dedup.push_back(off);
            }

            succinct::bit_vector_builder enc_tok_offsets;
            const std::pair<uint64_t, uint64_t> res = partitioned_ef_encode(enc_tok_offsets, offsets_dedup);
            const uint64_t num_bits = enc_tok_offsets.size();

            const Pef_Header header = {num_bits, res.first, res.second};

            succinct::bit_vector fin(&enc_tok_offsets);

            id_to_offset.push_back(offset);

            offset += sizeof(header) + fin.data().size() * sizeof(uint64_t);
        }
    } else {
        throw std::invalid_argument("Encoding type not supported");
    }

    bool empty = id_to_offset.size() == 0;
    bool except = false;

    std::vector<uint64_t> input;
    
    try {
        input = get_id_offsets_from_file(file_name, type, enc_type);
    } catch (const std::invalid_argument &e) {
        except = true;
    }

    const std::string type_string = token_type_to_str(type);

    bool ret;
    if (input == id_to_offset || (empty && except)) {
        ret = true;
        std::cout << type_string << " ID offsets correct: " << file_name << std::endl;
    } else {
        ret = false;
        std::cout << type_string << " ID offsets incorrect: " << file_name << std::endl;
                std::cout << "Offsets OG:" << std::endl;
                for (auto i : id_to_offset) {
                    std::cout << i << std::endl;
                }

                std::cout << "from file:" << std::endl;
                for (auto i : input) {
                    std::cout << i << std::endl;
                }
    }

    offset = 1;
    uint64_t id = 0;
    empty = false;
    except = false;
    bool correct = true;
    for (auto it = tokens_map.begin(); it != tokens_map.end(); ++it) {
        std::vector<uint64_t> offsets;
        
        // Convert offsets to 0 indexing and remove duplicates
        for (auto &i : it->second) {
            if (offsets.size() && offsets.back() == i-1) continue;
            offsets.push_back(i - 1);
        }

        std::vector<uint64_t> from_file;
        try {
             from_file = get_tok_offsets_from_file(id, id_to_offset, file_name, type, enc_type);
        } catch (const std::invalid_argument &e) {
            std::cout << "here" << std::endl;
            except = true;
        }
         
        correct = correct && (from_file == offsets);

        // if (from_file != offsets) {
        //     std::cout << "Offsets OG:" << std::endl;
        //     for (auto i : offsets) {
        //         std::cout << i << std::endl;
        //     }

        //     std::cout << "from file:" << std::endl;
        //     for (auto i : from_file) {
        //         std::cout << i << std::endl;
        //     }
        // }

        ++id;
    }

    if (correct || (empty && except)) {
        std::cout << type_string << " Token offsets correct: " << file_name << std::endl;
        return ret && true;
    } else {
        std::cout << type_string << " Token offsets incorrect: " << file_name << std::endl;
       return false;
    }
}

#endif