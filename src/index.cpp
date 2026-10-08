#include "tokenizer.h"
#include "simdbp128.h"
#include "index.h"
#include "coder.h"
#include "fst.h"
#include "helper.h"
#include "crc.h"
#include "fls.h"
#include "pef.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;

void create_index(const std::vector<std::string> &fields,
                  const std::string &index_dir_str,
                  const std::string &data_dir_str, 
                  const uint8_t enc_type,
                  const bool run_checks) {
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
  for (const auto &entry : std::filesystem::directory_iterator(data_dir)) {
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
    const uint8_t type = get_file_type(f.extension());
    Tokenizer tokenizer = Tokenizer::from_file(data_dir_str + '/' + f.filename().string(), type, fields);
    tokenizer.printStatistics();

    const std::string file_id = std::to_string(i++);

    generate_offsets_file(tokenizer, offsets_dir.string() + '/' + file_id, enc_type);
    generate_fst_files(tokenizer, fst_dir.string(), file_id);
  }

  create_fst<std::string>(file_mappings_vec, index_dir.string() + "/" + "file_mappings.fst");

  if (run_checks)
    check_fst<std::string>(file_mappings_vec, index_dir.string() + "/" + "file_mappings.fst");

  create_checksum_file(index_dir_str);
}

void generate_fst_files(Tokenizer &tokenizer, const std::string &fst_dir, const std::string &file_id, const bool run_checks) {
    std::cout << "Creating Alpha FST" << std::endl;
    create_fst<std::string>(tokenizer.getAlphaTokens(), fst_dir + "/" + file_id + "_alpha.fst");
    
    if (run_checks)
      check_fst(tokenizer.getAlphaTokens(), fst_dir + "/" + file_id + "_alpha.fst");
    
      tokenizer.alpha_tokens.clear();
    std::cout << "Finished" << std::endl;

    std::cout << "Creating Float FST" << std::endl;
    create_fst<std::vector<uint8_t>>(tokens_from_map<std::vector<uint8_t>>(tokenizer.float_tokens),
        fst_dir + "/" + file_id + "_float.fst");
    
    if (run_checks)
      check_fst(tokenizer.getFloatTokens(), fst_dir + "/" + file_id + "_float.fst");
    
      tokenizer.float_tokens.clear();
    std::cout << "Finished" << std::endl;
    
    create_fst<std::string>(tokenizer.fields, fst_dir + "/" + file_id + "_fields.fst");

    if (run_checks)
      check_fst(tokenizer.fields, fst_dir + "/" + file_id + "_fields.fst");

    tokenizer.fields.clear();
    std::cout << "Finished" << std::endl;
}

void generate_offsets_file(const Tokenizer &tokenizer,
                           const std::string &file_name,
                           const uint8_t enc_type,
                           const bool run_checks) {
  std::ofstream file;
  file.open(file_name + ".off", std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + file_name);
  }

  Offsets_header header = {};
  file.write(reinterpret_cast<const char *>(&header),
             sizeof(header)); // Reserve for later use

  const Write_pos alpha_pos =
      write_offsets_to_file(tokenizer.alpha_tokens, file, enc_type);
  const Write_pos num_pos =
      write_offsets_to_file(tokenizer.num_tokens, file, enc_type);
  const Write_pos neg_num_pos =
      write_offsets_to_file(tokenizer.neg_num_tokens, file, enc_type);
  const Write_pos float_pos =
      write_offsets_to_file(tokenizer.float_tokens, file, enc_type);
  const Write_pos date_pos =
      write_offsets_to_file(tokenizer.d_tokens, file, enc_type);
  const Write_pos time_pos =
      write_offsets_to_file(tokenizer.t_tokens, file, enc_type);
  const Write_pos ipv4_pos =
      write_offsets_to_file(tokenizer.ipv4_tokens, file, enc_type);
  const uint64_t entry_offsets_pos = write_entry_offsets_to_file(
      tokenizer.entry_offsets, file, enc_type); // Handled differently to above

  const uint64_t num_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getNumTokens(), file);
  const uint64_t neg_num_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getNegNumTokens(), file);
  const uint64_t date_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getDateTokens(), file);
  const uint64_t time_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getTimeTokens(), file);
  const uint64_t ip_tok_off =
      create_pef_tokens<uint64_t>(tokenizer.getIPTokens(), file);

  // Populate header with offset values then write to file
  header = {alpha_pos.offsets_pos,
            alpha_pos.id_map_pos,
            num_pos.offsets_pos,
            num_pos.id_map_pos,
            neg_num_pos.offsets_pos,
            neg_num_pos.id_map_pos,
            float_pos.offsets_pos,
            float_pos.id_map_pos,
            date_pos.offsets_pos,
            date_pos.id_map_pos,
            time_pos.offsets_pos,
            time_pos.id_map_pos,
            ipv4_pos.offsets_pos,
            ipv4_pos.id_map_pos,
            entry_offsets_pos,
            num_tok_off,
            neg_num_tok_off,
            date_tok_off,
            time_tok_off,
            ip_tok_off};

  file.seekp(0);
  file.write(reinterpret_cast<const char *>(&header),
             sizeof(header)); // Write header data

  file.close();

  if (run_checks) {
    check_offsets_file(tokenizer, file_name + ".off", enc_type);
    check_all_pef_tokens(tokenizer, file_name + ".off");
  }
}

void check_all_pef_tokens(const Tokenizer &tokenizer,
                          const std::string &file_path) {
  check_pef_tokens<uint64_t>(tokenizer.getNumTokens(), file_path,
                             TokenType::PINT);
  check_pef_tokens<uint64_t>(tokenizer.getNegNumTokens(), file_path,
                             TokenType::NINT);
  check_pef_tokens<uint64_t>(tokenizer.getDateTokens(), file_path,
                             TokenType::DATE);
  check_pef_tokens<uint64_t>(tokenizer.getTimeTokens(), file_path,
                             TokenType::TIME);
  check_pef_tokens<uint64_t>(tokenizer.getIPTokens(), file_path,
                             TokenType::IPV4);
}

const uint64_t
write_entry_offsets_to_file(const std::vector<uint64_t> &entry_offsets,
                            std::ofstream &file, const uint8_t enc_type) {
  const uint64_t write_pos = file.tellp();                            
  
  if (entry_offsets.size() == 0)
    return write_pos;

  switch (enc_type) {
    case EncType::GVB:
      write_gvb_to_file(entry_offsets, file);
      break;
    case EncType::PEF:
      write_pef_to_file(entry_offsets, file);
      break;
    case EncType::FLS:
      write_fls_to_file(entry_offsets, file);
      break;
    case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
    case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
      write_bp128_to_file(entry_offsets, file, enc_type);
      break;
    default:
      throw std::invalid_argument("Not a valid encoding type");
      break;
  }

  return write_pos;
}

void check_offsets_file(const Tokenizer &tokenizer,
                        const std::string &file_name, const uint8_t enc_type) {
  bool check = true;
  check = check && check_offsets_file_by_type(tokenizer.alpha_tokens, file_name, TokenType::ALPHA, enc_type);
  check = check && check_offsets_file_by_type(tokenizer.num_tokens, file_name, TokenType::PINT ,enc_type);
  check = check && check_offsets_file_by_type(tokenizer.neg_num_tokens, file_name, TokenType::NINT ,enc_type);
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

const bool check_entry_offsets(const std::vector<uint64_t> &entry_offsets,
                               const std::string &file_name,
                               const uint8_t enc_type) {
  // Convert into 0 indexing
  std::vector<uint64_t> entry_offsets_0;
  for (const auto cur : entry_offsets) {
    entry_offsets_0.push_back(cur - 1);
  }

  std::vector<uint64_t> input =
      get_entry_offsets_from_file(file_name, enc_type);

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

const std::pair<uint64_t, uint64_t> get_id_offsets_setup(const Offsets_header &header, const uint8_t type) {
  uint64_t addr_offset = 0;
  uint64_t size_to_read = 0;

  switch (type) {
  case TokenType::ALPHA:
    size_to_read = header.num_offsets_header - header.alpha_id_map_header;
    addr_offset = header.alpha_id_map_header;
    break;
  case TokenType::PINT:
    size_to_read = header.neg_num_offsets_header - header.num_id_map_header;
    addr_offset = header.num_id_map_header;
    break;
  case TokenType::NINT:
    size_to_read = header.float_offsets_header - header.neg_num_id_map_header;
    addr_offset = header.neg_num_id_map_header;
    break;
  case TokenType::FLOAT:
    size_to_read = header.date_offsets_header - header.float_id_map_header;
    addr_offset = header.float_id_map_header;
    break;
  case TokenType::DATE:
    size_to_read = header.time_offsets_header - header.date_id_map_header;
    addr_offset = header.date_id_map_header;
    break;
  case TokenType::TIME:
    size_to_read = header.ipv4_offsets_header - header.time_id_map_header;
    addr_offset = header.time_id_map_header;
    break;
  case TokenType::IPV4:
    size_to_read = header.entry_offsets_header - header.ipv4_id_map_header;
    addr_offset = header.ipv4_id_map_header;
    break;
  default:
    throw std::invalid_argument("Not a valid TokenType");
  }

  return std::make_pair(addr_offset, size_to_read);
}

const std::vector<uint64_t>
get_id_offsets_from_file(const std::string &file_name, const uint8_t type,
                         const uint8_t enc_type) {
  const MappedFile mf = open_mmap_r(file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();
  // Read in header
  Offsets_header header = {};
  std::memcpy(reinterpret_cast<char *>(&header), &file[0], sizeof(header));

  const auto &[addr_offset, size_to_read] = get_id_offsets_setup(header, type);

  if (!size_to_read)
    return std::vector<uint64_t>(); // No ids of requested type present in file

  switch (enc_type) {
    case EncType::GVB:
      return read_gvb_from_file(file + addr_offset, size_to_read);
    case EncType::PEF:
      return read_pef_from_file(file + addr_offset, size_to_read);
    case EncType::FLS: 
      return read_fls_from_file(file + addr_offset, size_to_read);
    case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
    case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
      return read_bp128_from_file(file + addr_offset, size_to_read, enc_type);
    default:
      throw std::invalid_argument("Not a valid encoding type");
  }
}

const std::vector<uint64_t> get_tok_offsets_from_file(
  const uint64_t token_id, const std::vector<uint64_t> &id_to_offset, const std::vector<uint64_t> &field_ids,
  const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
  const MappedFile mf = open_mmap_r(file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();
    
  const auto [length, offset] = get_tok_offsets_setup(id_to_offset, file, type);
  return get_tok_offsets(token_id, id_to_offset, field_ids, length, file + offset, enc_type);
}

const std::vector<uint64_t> get_tok_offsets_from_file(
  const Range token_ids, const std::vector<uint64_t> &id_to_offset, const std::vector<uint64_t> &field_ids,
  const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
  const MappedFile mf = open_mmap_r(file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();

  const auto [length, offset] = get_tok_offsets_setup(id_to_offset, file, type);

  std::vector<uint64_t> offsets;

  const auto [min, max] = token_ids;
  for (size_t id = min; id <= max; ++id) {
    std::vector<uint64_t> cur_offsets =
        get_tok_offsets(id, id_to_offset, field_ids, length, file + offset, enc_type);
    offsets.insert(offsets.end(), std::make_move_iterator(cur_offsets.begin()),
                   std::make_move_iterator(cur_offsets.end()));
  }

  std::sort(offsets.begin(), offsets.end());

  return offsets;
}

const std::pair<uint64_t, uint64_t>
get_tok_offsets_setup(const std::vector<uint64_t> &id_to_offset,
                      const uint8_t *file, const uint8_t type) {
  // Read in header
  Offsets_header header = {};
  std::memcpy(reinterpret_cast<char *>(&header), &file[0], sizeof(header));

  // Get offset to start of token offsets in file
  uint64_t tok_offsets_length = 0;
  uint64_t offset = 0;

  switch (type) {
    case TokenType::ALPHA:
      tok_offsets_length = header.alpha_id_map_header - header.alpha_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No alpha offsets present in file");
      offset = header.alpha_offsets_header;
      break;
    case TokenType::PINT:
      tok_offsets_length = header.num_id_map_header - header.num_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No int offsets present in file");
      offset = header.num_offsets_header;
      break;
    case TokenType::NINT:
      tok_offsets_length = header.neg_num_id_map_header - header.neg_num_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No int offsets present in file");
      offset = header.neg_num_offsets_header;
      break;
    case TokenType::FLOAT:
      tok_offsets_length = header.float_id_map_header - header.float_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No float offsets present in file");
      offset = header.float_offsets_header;
      break;
    case TokenType::DATE:
      tok_offsets_length = header.date_id_map_header - header.date_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No date offsets present in file");
      offset = header.date_offsets_header;
      break;
    case TokenType::TIME:
      tok_offsets_length = header.time_id_map_header - header.time_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No time offsets present in file");
      offset = header.time_offsets_header;
      break;
    case TokenType::IPV4:
      tok_offsets_length = header.ipv4_id_map_header - header.ipv4_offsets_header;
      if (tok_offsets_length == 0)
        throw std::invalid_argument("No ipv4 offsets present in file");
      offset = header.ipv4_offsets_header;
      break;
    default:
      throw std::invalid_argument("Not a valid token type");
  }

  return {tok_offsets_length, offset};
}

// Output converts offsets values from file from 1 indexing back to 0 indexing
const std::vector<uint64_t>
get_tok_offsets(const uint64_t token_id,
                const std::vector<uint64_t> &id_to_offset,
                const std::vector<uint64_t> &req_field_ids,
                const uint64_t tok_offsets_length,
                const uint8_t *file, const uint8_t enc_type) {
  uint64_t size_to_read;
  if (token_id == id_to_offset.size() - 1) {
    size_to_read = tok_offsets_length - id_to_offset[token_id] + 1;
  } else {
    size_to_read = id_to_offset[token_id + 1] - id_to_offset[token_id];
  }

  uint64_t addr_offset = id_to_offset[token_id] - 1;

  Fields_header f_header = {};
  // Get fields header
  std::memcpy(&f_header.num_fields, file + addr_offset, sizeof(uint16_t));
  size_t f_header_offset = addr_offset + sizeof(uint16_t);

  // Read field_ids
  f_header.field_ids.resize(f_header.num_fields);
  std::memcpy(f_header.field_ids.data(), file + f_header_offset, f_header.num_fields * sizeof(uint16_t));
  f_header_offset += f_header.num_fields * sizeof(uint16_t);

  // Read field_offsets
  f_header.field_offsets.resize(f_header.num_fields);
  std::memcpy(f_header.field_offsets.data(), file + f_header_offset, f_header.num_fields * sizeof(uint32_t));

  std::vector<uint64_t> field_sizes;
  for (uint64_t i = 0; i < f_header.num_fields - 1; ++i) {
    field_sizes.push_back(f_header.field_offsets[i+1] - f_header.field_offsets[i]);
  }

  field_sizes.push_back(size_to_read - (f_header.field_offsets[f_header.num_fields - 1] - id_to_offset[token_id]));

  std::vector<uint64_t> all_docids;
  for (uint64_t i = 0; i < f_header.field_ids.size(); ++i) {

    // Continue if requesting specific field_ids but not found in postings list
    if (!req_field_ids.empty()) {
      if (!std::binary_search(req_field_ids.begin(), req_field_ids.end(), f_header.field_ids[i]))
        continue;
    }

    addr_offset = f_header.field_offsets[i] - 1;

    std::vector<uint64_t> docids;
    switch (enc_type) {
      case EncType::GVB:
        docids = read_gvb_from_file(file + addr_offset, field_sizes[i]);
        break;
      case EncType::PEF:
        docids = read_pef_from_file(file + addr_offset, field_sizes[i]);
        break;
      case EncType::FLS:
        docids = read_fls_from_file(file + addr_offset, field_sizes[i]);
        break;
      case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
      case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
        docids = read_bp128_from_file(file + addr_offset, field_sizes[i], enc_type);
        break;
      default:
        throw std::invalid_argument("Not a valid encoding type");
    }

    std::vector<uint64_t> merged;
    merged.reserve(docids.size() + all_docids.size());
    std::merge(docids.begin(), docids.end(), all_docids.begin(), all_docids.end(), std::back_inserter(merged));
    all_docids = merged;
  }

  // Convert back to 0 indexing
  for (auto &o : all_docids)
    --o;

  std::vector<uint64_t> dedup = remove_dups_sorted(all_docids);

  return dedup;
}

const std::vector<uint64_t> read_entry_offsets_gvb(const uint8_t *file, const size_t size_to_read) {
  std::vector<uint64_t> decoded = read_gvb_from_file(file, size_to_read);

  // Convert to 0 indexing
  for (auto &o : decoded)
    --o;

  return decoded;
}

const std::vector<uint64_t> read_entry_offsets_pef(const uint8_t *file, const size_t size_to_read) {
  std::vector<uint64_t> decoded = read_pef_from_file(file, size_to_read);

  // Convert back to 0 indexing
  for (auto &o : decoded)
    --o;

  return decoded;
}

const std::vector<uint64_t> read_entry_offsets_fls(const uint8_t *file, const size_t size_to_read) {
  std::vector<uint64_t> decoded = read_fls_from_file(file, size_to_read);

  // Convert to 0 indexing
  for (auto &o : decoded)
    --o;

  return decoded;
}

const std::vector<uint64_t> read_entry_offsets_bp128(const uint8_t *file, const size_t size_to_read, uint8_t enc_type) {
  std::vector<uint64_t> decoded = read_bp128_from_file(file, size_to_read, enc_type);

  // Convert to 0 indexing
  for (auto &o : decoded)
    --o;

  return decoded;
}

// Output converts offsets values from file from 1 indexing back to 0 indexing
const std::vector<uint64_t>
get_entry_offsets_from_file(const std::string &file_name,
                            const uint8_t enc_type) {
  const MappedFile mf = open_mmap_r(file_name);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();

  // Read in header
  Offsets_header header = {};
  std::memcpy(reinterpret_cast<char *>(&header), &file[0], sizeof(header));

  const size_t size_to_read =
      header.int_toks_header - header.entry_offsets_header;

  if (size_to_read == 0)
    throw std::invalid_argument("No entry offsets present in file");

  switch (enc_type) {
  case EncType::GVB:
    return read_entry_offsets_gvb(file + header.entry_offsets_header, size_to_read);
  case EncType::PEF:
    return read_entry_offsets_pef(file + header.entry_offsets_header, size_to_read);
  case EncType::FLS:
    return read_entry_offsets_fls(file + header.entry_offsets_header, size_to_read);
  case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
  case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
    return read_entry_offsets_bp128(file + header.entry_offsets_header, size_to_read, enc_type);
  default:
    throw std::invalid_argument("Not a valid encoding type");
  }
}

std::pair<uint64_t, uint64_t>
get_pef_header_details_by_type(const uint8_t *file, const uint64_t file_size,
                               const uint8_t type) {
  // Read in header
  Offsets_header header = {};
  std::memcpy(reinterpret_cast<char *>(&header), &file[0], sizeof(header));

  uint64_t offset = 0;
  uint64_t size_to_read = 0;
  switch (type) {
  case TokenType::PINT:
    offset = header.int_toks_header;
    size_to_read = header.neg_int_toks_header - header.int_toks_header;
    break;
  case TokenType::NINT:
    offset = header.neg_int_toks_header;
    size_to_read = header.date_toks_header - header.neg_int_toks_header;
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
  default:
    throw std::invalid_argument("Type not valid here");
  }

  return {offset, size_to_read};
}

void check_index_dir_valid(const std::string &index_path) {
  // Check valid fst directory
  if (!std::filesystem::exists(index_path + "/fsts"))
    throw std::invalid_argument("Fst's directory does not exist: " + index_path + "/fsts");

  // Check valid offsets directory
  if (!std::filesystem::exists(index_path + "/offsets"))
    throw std::invalid_argument("Offset's directory does not exist: " + index_path + "/offsets");

  // Check file_mappings file exists
  if (!std::filesystem::exists(index_path + "/file_mappings.fst"))
    throw std::invalid_argument("file_mappings file does not exist: " + index_path + "/file_mappings.fst");
}