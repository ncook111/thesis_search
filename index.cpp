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

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;

void create_index(const std::string &index_dir_str, const std::string &data_dir_str, const uint8_t enc_type) {
    const std::filesystem::path index_dir = index_dir_str;
    const std::filesystem::path offsets_dir = index_dir_str + "/offsets";
    const std::filesystem::path fst_dir = index_dir_str + "/fsts";
    const std::filesystem::path data_dir = data_dir_str;

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

    // Must be sorted for later fst creation
    std::sort(files.begin(), files.end());

    std::vector<std::string> file_mappings_vec;

    // Tokenize, generate offsets and fst's for all supported files
    size_t i = 0;
    for (const auto &f : files) {
        file_mappings_vec.push_back(f.filename().string());
        std::cout << f.filename().string() << std::endl;

        const uint8_t type = get_file_type(f.extension());
        Tokenizer tokenizer = Tokenizer::from_file(data_dir_str + '/' + f.filename().string(), type);
        tokenizer.printStatistics();

        const std::string file_id = std::to_string(i);

        generate_offsets_file(tokenizer, offsets_dir.string() + '/' + file_id, enc_type);

        std::cout << "Creating Alpha FST" << std::endl;
        create_fst<std::string>(tokenizer.getAlphaTokens(), fst_dir.string() + "/" + file_id + "_alpha.fst");
        check_fst(tokenizer.getAlphaTokens(), fst_dir.string() + "/" + file_id + "_alpha.fst");
        tokenizer.alpha_tokens.clear();
        std::cout << "Finished" << std::endl;

        std::cout << "Creating Float FST" << std::endl;
        create_fst<std::vector<uint8_t>>(tokens_from_map<std::vector<uint8_t>>(tokenizer.float_tokens),  fst_dir.string() + "/" + file_id + "_float.fst");
        check_fst(tokenizer.getFloatTokens(), fst_dir.string() + "/" + file_id + "_float.fst");
        tokenizer.float_tokens.clear();
        std::cout << "Finished" << std::endl;

        ++i;
    }

    create_fst<std::string>(file_mappings_vec, index_dir.string() + "/" + "file_mappings.fst");
    check_fst<std::string>(file_mappings_vec, index_dir.string() + "/" + "file_mappings.fst");
}

const std::vector<uint64_t> read_id_offsets_gvb(const uint8_t *file, const uint64_t addr_offset, const uint64_t size_to_read) {
    std::vector<uint8_t> enc_id_to_offset(size_to_read);
    std::memcpy(reinterpret_cast<char*>(enc_id_to_offset.data()), &file[addr_offset], size_to_read);

    const std::vector<uint64_t> id_to_offset = gvb_decode_uint64_t(enc_id_to_offset);

    return id_to_offset;
}

const std::vector<uint64_t> read_id_offsets_pef(const uint8_t *file, const uint64_t addr_offset, const uint64_t size_to_read) {
    // Read in universe and n first
    const Pef_Header header = get_bv_header(file, addr_offset);
    const succinct::bit_vector bv = get_bv(file, header, addr_offset + sizeof(header), size_to_read - sizeof(header));
    

    return partitioned_ef_decode(bv, header.universe, header.n);
}

const std::vector<uint64_t> get_id_offsets_from_file(const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
    const MappedFile mf = open_mmap_r(file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();

    // Read in header
    Offsets_header header = {};
    std::memcpy(reinterpret_cast<char*>(&header), &file[0], sizeof(header));

    size_t size_to_read = 0;
    size_t addr_offset = 0;

    // Should only ever be requesting id_offsets if present?
    switch (type) {
        case TokenType::ALPHA:
            size_to_read = header.alpha_offsets_header - header.alpha_id_map_header;
            addr_offset = header.alpha_id_map_header;
            break;
        case TokenType::INT:
            size_to_read = header.num_offsets_header - header.num_id_map_header;
            addr_offset = header.num_id_map_header;
            break;
        case TokenType::FLOAT:
            size_to_read = header.float_offsets_header - header.float_id_map_header;
            addr_offset = header.float_id_map_header;
            break;
        case TokenType::DATE:
            size_to_read = header.date_offsets_header - header.date_id_map_header;
            addr_offset = header.date_id_map_header;
            break;
        case TokenType::TIME:
            size_to_read = header.time_offsets_header - header.time_id_map_header;
            addr_offset = header.time_id_map_header;
            break;
        case TokenType::IPV4:
            size_to_read = header.ipv4_offsets_header - header.ipv4_id_map_header;
            addr_offset = header.ipv4_id_map_header;
            break;
        default:
            break;
    }

    if (!size_to_read) return std::vector<uint64_t>(); // No ids of requested type present in file

    switch (enc_type) {
        case EncType::GVB:
            return read_id_offsets_gvb(file, addr_offset, size_to_read);
            break;
        case EncType::PEF:
            return read_id_offsets_pef(file, addr_offset, size_to_read);
            break;
        default:
            throw std::invalid_argument("Not a valid encoding type");
    }
}

const std::vector<uint64_t> read_tok_offsets_gvb(const uint8_t *file, const uint64_t addr_offset, const uint64_t size_to_read) {
    std::vector<uint8_t> enc_tok_offsets(size_to_read);
    std::memcpy(reinterpret_cast<char*>(enc_tok_offsets.data()), &file[addr_offset], size_to_read);
    std::vector<uint64_t> offsets = delta_decode(gvb_decode_uint64_t(enc_tok_offsets));
    
    // Convert back to 0 indexing
    for (auto &o : offsets) --o;

    return offsets;
}

const std::vector<uint64_t> read_tok_offsets_pef(const uint8_t *file, const uint64_t addr_offset, const uint64_t size_to_read) {
    // Read in universe and n first
    const Pef_Header header = get_bv_header(file, addr_offset);
    const succinct::bit_vector bv = get_bv(file, header, addr_offset + sizeof(header), size_to_read - sizeof(header));

    std::vector<uint64_t> offsets = partitioned_ef_decode(bv, header.universe, header.n);
    
    // Convert back to 0 indexing
    for (auto &o : offsets) --o;

    return offsets;
}

const std::vector<uint64_t> get_tok_offsets_from_file(const uint64_t token_id, const std::vector<uint64_t> &id_to_offset, const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
    const MappedFile mf = open_mmap_r(file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();
    
    const auto [length, offset] = get_tok_offsets_setup(id_to_offset, file, type);

    return get_tok_offsets(token_id, id_to_offset, length, offset, file, enc_type);
}

const std::vector<uint64_t> get_tok_offsets_from_file(const Range token_ids, const std::vector<uint64_t> &id_to_offset, const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
    const MappedFile mf = open_mmap_r(file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();
    
    const auto [length, offset] = get_tok_offsets_setup(id_to_offset, file, type);

    std::vector<uint64_t> offsets;
    
    const auto [min, max] = token_ids;
    for (size_t id = min; id <= max; ++id) {
        std::vector<uint64_t> cur_offsets = get_tok_offsets(id, id_to_offset, length, offset, file, enc_type);
        offsets.insert(offsets.end(), std::make_move_iterator(cur_offsets.begin()), std::make_move_iterator(cur_offsets.end()));
    }
    
    return offsets;
}

const std::pair<uint64_t, uint64_t> get_tok_offsets_setup(const std::vector<uint64_t> &id_to_offset, const uint8_t* file, const uint8_t type) {
    // Read in header
    Offsets_header header = {};
    std::memcpy(reinterpret_cast<char*>(&header), &file[0], sizeof(header));

    // Get offset to start of token offsets in file
    uint64_t size_to_read = 0;
    uint64_t tok_offsets_length = 0;
    uint64_t offset = 0;

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

    return {tok_offsets_length, offset};
}


// Output converts offsets values from file from 1 indexing back to 0 indexing
const std::vector<uint64_t> get_tok_offsets(const uint64_t token_id, const std::vector<uint64_t> &id_to_offset, const uint64_t tok_offsets_length, const uint64_t offset, const uint8_t* file, const uint8_t enc_type) {
    uint64_t size_to_read;
    if (token_id == id_to_offset.size() - 1) {
        size_to_read = tok_offsets_length - id_to_offset[token_id] + 1;
    } else {
        size_to_read = id_to_offset[token_id+1] - id_to_offset[token_id];
    } 

    switch (enc_type) {
        case EncType::GVB:
            return read_tok_offsets_gvb(file, offset + id_to_offset[token_id]-1, size_to_read);
            break;
        case EncType::PEF:
            return read_tok_offsets_pef(file, offset + id_to_offset[token_id]-1, size_to_read);
            break;
        default:
            throw std::invalid_argument("Not a valid encoding type");
    }
}

const std::vector<uint64_t> read_entry_offsets_gvb(const uint8_t *file, const size_t addr_offset, const size_t size_to_read) {
    std::vector<uint8_t> enc_entry_offsets_vec(size_to_read);
    std::memcpy(reinterpret_cast<char*>(enc_entry_offsets_vec.data()), &(file[addr_offset]), size_to_read);

    std::vector<uint64_t> entry_offsets_vec = gvb_decode_uint64_t(enc_entry_offsets_vec);

    // Convert to 0 indexing
    for (auto &o : entry_offsets_vec) --o;

    return entry_offsets_vec;
}

const std::vector<uint64_t> read_entry_offsets_pef(const uint8_t *file, const size_t addr_offset, const size_t size_to_read) {
    // Read in universe and n first
    const Pef_Header header = get_bv_header(file, addr_offset);
    const succinct::bit_vector bv = get_bv(file, header, addr_offset + sizeof(header), size_to_read - sizeof(header));

    std::vector<uint64_t> offsets = partitioned_ef_decode(bv, header.universe, header.n);
    
    // Convert back to 0 indexing
    for (auto &o : offsets) --o;

    return offsets;
}

// Output converts offsets values from file from 1 indexing back to 0 indexing
const std::vector<uint64_t> get_entry_offsets_from_file(const std::string &file_name, const uint8_t enc_type) {
    const MappedFile mf = open_mmap_r(file_name);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();

    // Read in header
    Offsets_header header = {};
    std::memcpy(reinterpret_cast<char*>(&header), &file[0], sizeof(header));

    const size_t size_to_read = header.int_toks_header - header.entry_offsets_header;

    if (size_to_read == 0) throw std::invalid_argument("No entry offsets present in file");


    switch (enc_type) {
        case EncType::GVB:
            return read_entry_offsets_gvb(file, header.entry_offsets_header, size_to_read);
            break;
        case EncType::PEF:
            return read_entry_offsets_pef(file, header.entry_offsets_header, size_to_read);
            break;
        default:
            throw std::invalid_argument("Not a valid encoding type");
    }
}

void generate_offsets_file(const Tokenizer &tokenizer, const std::string &file_name, const uint8_t enc_type) {
    std::ofstream file;
    file.open(file_name + ".off", std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_name);
    }

    Offsets_header header = {};
    file.write(reinterpret_cast<const char*>(&header), sizeof(header)); // Reserve for later use

    const Write_pos alpha_pos = write_offsets_to_file(tokenizer.alpha_tokens, file, enc_type);
    const Write_pos num_pos = write_offsets_to_file(tokenizer.num_tokens, file, enc_type);
    const Write_pos float_pos = write_offsets_to_file(tokenizer.float_tokens, file, enc_type);
    const Write_pos date_pos = write_offsets_to_file(tokenizer.d_tokens, file, enc_type);
    const Write_pos time_pos = write_offsets_to_file(tokenizer.t_tokens, file, enc_type);
    const Write_pos ipv4_pos = write_offsets_to_file(tokenizer.ipv4_tokens, file, enc_type);
    const uint64_t entry_offsets_pos = write_entry_offsets_to_file(tokenizer.entry_offsets, file, enc_type); // Handled differently to above

    const uint64_t num_tok_off = create_pef_tokens<uint64_t>(tokenizer.getNumTokens(), file);
    const uint64_t date_tok_off = create_pef_tokens<uint64_t>(tokenizer.getDateTokens(),  file);
    const uint64_t time_tok_off = create_pef_tokens<uint64_t>(tokenizer.getTimeTokens(),  file);
    const uint64_t ip_tok_off = create_pef_tokens<uint64_t>(tokenizer.getIPTokens(), file);



    // Populate header with offset values then write to file   
    header = {
        alpha_pos.id_map_pos, alpha_pos.offsets_pos,
        num_pos.id_map_pos, num_pos.offsets_pos,
        float_pos.id_map_pos, float_pos.offsets_pos,
        date_pos.id_map_pos, date_pos.offsets_pos,
        time_pos.id_map_pos, time_pos.offsets_pos,
        ipv4_pos.id_map_pos, ipv4_pos.offsets_pos,
        entry_offsets_pos,
        num_tok_off,
        date_tok_off,
        time_tok_off,
        ip_tok_off
    };

    file.seekp(0); 
    file.write(reinterpret_cast<const char*>(&header), sizeof(header)); // Write header data

    file.close();

    //check_offsets_file(tokenizer, file_name + ".off", enc_type);
    //check_all_pef_tokens(tokenizer, file_name + ".off");
}

void check_all_pef_tokens(const Tokenizer &tokenizer, const std::string &file_path) {
    check_pef_tokens<uint64_t>(tokenizer.getNumTokens(), file_path, TokenType::INT);
    check_pef_tokens<uint64_t>(tokenizer.getDateTokens(), file_path, TokenType::DATE);
    check_pef_tokens<uint64_t>(tokenizer.getTimeTokens(), file_path, TokenType::TIME);
    check_pef_tokens<uint64_t>(tokenizer.getIPTokens(), file_path, TokenType::IPV4);
}

const uint64_t write_entry_offsets_gvb(const std::vector<uint64_t> &entry_offsets, std::ofstream &file) {
    const std::vector<uint8_t> all_enc_entry_offsets = gvb_encode_uint64_t(entry_offsets);

    // Write to file
    const uint64_t entry_offsets_size = all_enc_entry_offsets.size();

    const uint64_t write_pos = file.tellp();
    file.write(reinterpret_cast<const char*>(all_enc_entry_offsets.data()), sizeof(uint8_t) * entry_offsets_size);

    return write_pos;
}

const uint64_t write_entry_offsets_pef(const std::vector<uint64_t> &entry_offsets, std::ofstream &file) {
    succinct::bit_vector enc_tok_offsets;
    const auto [universe, n] = partitioned_ef_encode(enc_tok_offsets, entry_offsets);
    const uint64_t num_bits = enc_tok_offsets.size();
    const Pef_Header header = {num_bits, universe, n};

    // Write to file
    const uint64_t entry_offsets_size = enc_tok_offsets.data().size() * sizeof(uint64_t);

    const uint64_t write_pos = file.tellp();

    // Write num_bits, universe and n
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write enc_tok_offsets
    file.write(reinterpret_cast<const char*>(enc_tok_offsets.data().data()), entry_offsets_size);

    return write_pos;
}

const uint64_t write_entry_offsets_to_file(const std::vector<uint64_t> &entry_offsets, std::ofstream &file, const uint8_t enc_type) {
    if (entry_offsets.size() == 0) { 
        return static_cast<uint64_t>(file.tellp());
    }
    
    switch (enc_type) {
        case EncType::GVB:
            return write_entry_offsets_gvb(entry_offsets, file);
        case EncType::PEF:
            return write_entry_offsets_pef(entry_offsets, file);
        default:
            throw std::invalid_argument("Not a valid encoding type");
    }

}

void check_offsets_file(const Tokenizer &tokenizer, const std::string &file_name, const uint8_t enc_type) {
    bool check = true;
    check = check && check_offsets_file_by_type(tokenizer.alpha_tokens, file_name, TokenType::ALPHA, enc_type);
    check = check && check_offsets_file_by_type(tokenizer.num_tokens, file_name, TokenType::INT ,enc_type);
    check = check && check_offsets_file_by_type(tokenizer.float_tokens, file_name, TokenType::FLOAT, enc_type);
    check = check && check_offsets_file_by_type(tokenizer.d_tokens, file_name, TokenType::DATE, enc_type);
    check = check && check_offsets_file_by_type(tokenizer.t_tokens, file_name, TokenType::TIME, enc_type);
    check = check && check_offsets_file_by_type(tokenizer.ipv4_tokens, file_name, TokenType::IPV4, enc_type);
    check = check && check_entry_offsets(tokenizer.entry_offsets, file_name, enc_type);

    if (check) {
        std::cout << file_name << ": Offsets file correct" << std::endl;
    } else {
        std::cout << file_name << ": Offsets file incorrect" << std::endl;
    }
}

const bool check_entry_offsets(const std::vector<uint64_t> &entry_offsets, const std::string &file_name, const uint8_t enc_type) {
    // Convert into 0 indexing
    std::vector<uint64_t> entry_offsets_0;
    for (const auto &cur : entry_offsets) {
        entry_offsets_0.push_back(cur - 1);
    }

    std::vector<uint64_t> input = get_entry_offsets_from_file(file_name, enc_type);

    if (input == entry_offsets_0) {
        std::cout << "Entry offsets correct: " << std::endl;
        return true;
    } else {
       std::cout << "Entry offsets incorrect: " << std::endl;
       std::cout << "read in size: " << input.size() << std::endl;
       std::cout << "og size: " << entry_offsets_0.size() << std::endl;
       return false;
    }
}

std::pair<uint64_t, uint64_t> get_pef_header_details_by_type(const uint8_t *file, const uint64_t file_size, const uint8_t type) {
    // Read in header
    Offsets_header header = {};
    std::memcpy(reinterpret_cast<char*>(&header), &file[0], sizeof(header));

    uint64_t offset = 0;
    uint64_t size_to_read = 0;
    switch (type) {
        case TokenType::INT: 
            offset = header.int_toks_header;
            size_to_read = header.date_toks_header - header.int_toks_header;
            break;
        case TokenType::DATE: 
            offset = header.date_toks_header;
            size_to_read = header.time_toks_header - header.date_toks_header;
            break;
        case TokenType::TIME: 
            offset = header.time_toks_header;
            size_to_read = header.ipv4_toks_header - header.time_toks_header;
            break;
        case TokenType::IPV4: 
            offset = header.ipv4_toks_header;
            size_to_read = file_size - header.ipv4_toks_header;
            break;
        default: throw std::invalid_argument("Type not valid here");
    }

    return {offset, size_to_read};
}

void check_index_dir_valid(const std::string &index_path) {
    // Check valid fst directory
    if (!std::filesystem::exists(index_path + "/fsts"))
        throw std::invalid_argument("Fst's directory does not exist");

    // Check valid offsets directory
    if (!std::filesystem::exists(index_path + "/offsets"))
        throw std::invalid_argument("Offset's directory does not exist");

    // Check file_mappings file exists
    if (!std::filesystem::exists(index_path + "/file_mappings.fst"))
        throw std::invalid_argument("file_mappings file does not exist");
}