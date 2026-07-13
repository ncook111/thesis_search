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
#include "pef.h"

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;

void create_index(const std::string &index_dir_str, const std::string &data_dir_str, const uint8_t enc_type);
const std::vector<uint64_t> get_id_offsets_from_file(const std::string &file_name, const uint8_t type, const uint8_t enc_type);
const std::vector<uint64_t> get_tok_offsets_from_file(const uint64_t token_id, const std::vector<uint64_t> &id_to_offset, const std::string &file_name, const uint8_t type, const uint8_t enc_type);
void generate_offsets_file(const Tokenizer &tokenizer, const std::string &file_name, const uint8_t enc_type);
void check_offsets_file(const Tokenizer &tokenizer, const std::string &file_name, const uint8_t enc_type);
const uint64_t write_entry_offsets_to_file(const std::vector<uint64_t> &entry_offsets, std::ofstream &file, const uint8_t enc_type);
const std::vector<uint64_t> get_entry_offsets_from_file(const std::string &file_namem, const uint8_t enc_type);
const bool check_entry_offsets(const std::vector<uint64_t> &entry_offsets, const std::string &file_name, const uint8_t enc_type);
const uint64_t write_entry_offsets_gvb(const std::vector<uint64_t> &entry_offsets, std::ofstream &file);
const uint64_t write_entry_offsets_pef(const std::vector<uint64_t> &entry_offsets, std::ofstream &file);
const std::vector<uint64_t> read_id_offsets_gvb(const uint8_t *file, const uint64_t addr_offset, const uint64_t size_to_read);
const std::vector<uint64_t> read_id_offsets_pef(const uint8_t *file, const uint64_t addr_offset, const uint64_t size_to_read);
const std::vector<uint64_t> read_tok_offsets_gvb(const uint8_t *file, const uint64_t addr_offset, const uint64_t size_to_read); 
const std::vector<uint64_t> read_tok_offsets_pef(const uint8_t *file, const uint64_t addr_offset, const uint64_t size_to_read);
const std::vector<uint64_t> read_entry_offsets_gvb(const uint8_t *file, const size_t addr_offset, const size_t size_to_read);
const std::vector<uint64_t> read_entry_offsets_pef(const uint8_t *file, const size_t addr_offset, const size_t size_to_read);
void check_all_pef_tokens(const Tokenizer &tokenizer, const std::string &file_path);
std::pair<uint64_t, uint64_t> get_pef_header_details_by_type(const uint8_t *file, const uint64_t file_size, const uint8_t type);
void check_index_dir_valid(const std::string &index_path);

// Saved to the front of the file
struct Offsets_header {
    uint64_t alpha_id_map_header, alpha_offsets_header;
    uint64_t num_id_map_header, num_offsets_header;
    uint64_t float_id_map_header, float_offsets_header;
    uint64_t date_id_map_header, date_offsets_header;
    uint64_t time_id_map_header, time_offsets_header;
    uint64_t ipv4_id_map_header, ipv4_offsets_header;
    uint64_t entry_offsets_header;
    uint64_t int_toks_header;
    uint64_t date_toks_header;
    uint64_t time_toks_header;
    uint64_t ipv4_toks_header;
};

struct Write_pos {
  uint64_t id_map_pos;
  uint64_t offsets_pos;
};

template <MapType M>
std::vector<M> read_pef_tokens(const std::string &file_path, const uint64_t offset, const uint64_t size_to_read) {
    const MappedFile mf = open_mmap_r(file_path);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();

    // Read in universe and n first
    const Pef_Header header = get_bv_header(file, offset);

    // If null return empty vec
    if (!header.n) {
        return std::vector<M>();
    }

    const succinct::bit_vector bv = get_bv(file, header, offset + sizeof(header), size_to_read - sizeof(header));

    const std::vector<uint64_t> offsets = partitioned_ef_decode(bv, header.universe, header.n);
    
    return offsets;
}

template <MapType M>
const uint64_t create_pef_tokens(const std::vector<M> &tokens, std::ofstream &file) {
    if (!tokens.size()) {
        const uint64_t pos = file.tellp();

        // Write null num_bits, universe and n then return
        const Pef_Header header = {};
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        return pos;
    }

    succinct::bit_vector pef_tokens;
    const std::pair<uint64_t, uint64_t> res = partitioned_ef_encode(pef_tokens, tokens);
    const uint64_t num_bits = pef_tokens.size();
    const Pef_Header header = {num_bits, res.first, res.second};

    const uint64_t pos = file.tellp();

    // Write to file
    const uint64_t pef_tokens_size = pef_tokens.data().size() * sizeof(uint64_t);

    // Write num_bits, universe and n
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write pef_tokens
    file.write(reinterpret_cast<const char*>(pef_tokens.data().data()), pef_tokens_size);

    return pos;
}

template <MapType M>
bool check_pef_tokens(const std::vector<M> &tokens, const std::string file_path, const uint8_t type) {
    const MappedFile mf = open_mmap_r(file_path);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();

    const std::pair<uint64_t, uint64_t> tok_details = get_pef_header_details_by_type(file, file_size, type);
    const std::vector<M> out = read_pef_tokens<M>(file_path, tok_details.first, tok_details.second);

    if (out == tokens) {
        std::cout << "correct" << std::endl;
        return true;
    } else {
        std::cout << "incorrect" << std::endl;
        return false;
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


        std::__1::unique_ptr<succinct::bit_vector> owned_enc_tok_offsets = std::make_unique<succinct::bit_vector>();
        const std::pair<uint64_t, uint64_t> res = partitioned_ef_encode(*owned_enc_tok_offsets, offsets_dedup);
        const uint64_t num_bits = owned_enc_tok_offsets.get()->size();

        const Pef_Header header = {num_bits, res.first, res.second};

        all_enc_tok_offsets.push_back(std::make_pair(header, std::move(owned_enc_tok_offsets)));

        id_to_offset.push_back(offset);

        offset += sizeof(header) + all_enc_tok_offsets.back().second->data().size() * sizeof(uint64_t);
    }

    succinct::bit_vector enc_id_to_offset;
    const std::pair<uint64_t, uint64_t> res = partitioned_ef_encode(enc_id_to_offset, id_to_offset);
    const uint64_t num_bits = enc_id_to_offset.size();
    const Pef_Header header = {num_bits, res.first, res.second};

    // Write to file
    const uint64_t first_write_pos = file.tellp();
    
    // Write num_bits, universe and n
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write id_to_offsets data
    uint64_t size = enc_id_to_offset.data().size() * sizeof(uint64_t);
    file.write(reinterpret_cast<const char*>(enc_id_to_offset.data().data()), size);


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
bool check_offsets_file_by_type(const std::map<M, std::vector<uint64_t>> &tokens_map, const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
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

            succinct::bit_vector enc_tok_offsets;
            const std::pair<uint64_t, uint64_t> res = partitioned_ef_encode(enc_tok_offsets, offsets_dedup);
            const uint64_t num_bits = enc_tok_offsets.size();

            const Pef_Header header = {num_bits, res.first, res.second};

            id_to_offset.push_back(offset);

            offset += sizeof(header) + enc_tok_offsets.data().size() * sizeof(uint64_t);
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

        if (from_file != offsets) {
            std::cout << "Offsets OG Size:" << offsets.size() << std::endl;
            std::cout << "from file Size:" << from_file.size() << std::endl;
            std::cout << "Offsets OG:" << std::endl;
            for (auto i : offsets) {
                std::cout << i << std::endl;
            }

            std::cout << "from file:" << std::endl;
            for (auto i : from_file) {
                std::cout << i << std::endl;
            }

            return false;
        }

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