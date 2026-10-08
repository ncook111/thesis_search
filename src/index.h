#ifndef INDEX_H
#define INDEX_H

#include "tokenizer.h"
#include "simdbp128.h"
#include "coder.h"
#include "custom_types.h"
#include "fst.h"
#include "fls.h"
#include "helper.h"
#include "pef.h"
#include <fstream>
#include <map>
#include <vector>

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::vector<uint64_t>> EntryMap;

// Saved to the front of the file
struct Offsets_header {
  uint64_t alpha_offsets_header, alpha_id_map_header; // Docids
  uint64_t num_offsets_header, num_id_map_header; // Docids
  uint64_t neg_num_offsets_header, neg_num_id_map_header; // Docids
  uint64_t float_offsets_header, float_id_map_header; // Docids
  uint64_t date_offsets_header, date_id_map_header; // Docids
  uint64_t time_offsets_header, time_id_map_header; // Docids
  uint64_t ipv4_offsets_header, ipv4_id_map_header; // Docids
  uint64_t entry_offsets_header; // File offsets for each document
  uint64_t int_toks_header; // PEF encoded lexicon values
  uint64_t neg_int_toks_header; // PEF encoded lexicon values
  uint64_t date_toks_header; // PEF encoded lexicon values
  uint64_t time_toks_header; // PEF encoded lexicon values
  uint64_t ipv4_toks_header; // PEF encoded lexicon values
};

struct Fields_header {
  uint16_t num_fields; // Number of fields that will contain docids
  std::vector<uint16_t> field_ids; // ID of fields
  std::vector<uint32_t> field_offsets; // File offset for which each fields docids will be stored
};

struct Write_pos {
  uint64_t offsets_pos;
  uint64_t id_map_pos;
};

void create_index(const std::vector<std::string> &fields,
                  const std::string &index_dir_str,
                  const std::string &data_dir_str,
                  const uint8_t enc_type,
                  const bool run_checks = false);
const std::vector<uint64_t> get_id_offsets_from_file(const std::string &file_name, const uint8_t type, const uint8_t enc_type);
const std::pair<uint64_t, uint64_t> get_id_offsets_setup(const Offsets_header &header, const uint8_t type);
const std::vector<uint64_t> get_tok_offsets_from_file(
    const uint64_t token_id, const std::vector<uint64_t> &id_to_offset, const std::vector<uint64_t> &field_ids,
    const std::string &file_name, const uint8_t type, const uint8_t enc_type);
const std::vector<uint64_t> get_tok_offsets_from_file(
    const Range token_ids, const std::vector<uint64_t> &id_to_offset, const std::vector<uint64_t> &field_ids,
    const std::string &file_name, const uint8_t type, const uint8_t enc_type);
const std::pair<uint64_t, uint64_t>
get_tok_offsets_setup(const std::vector<uint64_t> &id_to_offset,
                      const uint8_t *file, const uint8_t type);
const std::vector<uint64_t>
get_tok_offsets(const uint64_t token_id,
                const std::vector<uint64_t> &id_to_offset,
                const std::vector<uint64_t> &req_field_ids,
                const uint64_t tok_offsets_length,
                const uint8_t *file, const uint8_t enc_type);
void generate_offsets_file(const Tokenizer &tokenizer,
                           const std::string &file_name,
                           const uint8_t enc_type,
                          const bool run_checks = false);
void check_offsets_file(const Tokenizer &tokenizer,
                        const std::string &file_name, const uint8_t enc_type);
void generate_fst_files(Tokenizer &tokenizer, const std::string &fst_dir, const std::string &file_id, const bool run_checks = false);
const uint64_t
write_entry_offsets_to_file(const std::vector<uint64_t> &entry_offsets,
                            std::ofstream &file, const uint8_t enc_type);
const std::vector<uint64_t>
get_entry_offsets_from_file(const std::string &file_name,
                            const uint8_t enc_type);
const bool check_entry_offsets(const std::vector<uint64_t> &entry_offsets,
                               const std::string &file_name,
                               const uint8_t enc_type);
const std::vector<uint64_t> read_entry_offsets_gvb(const uint8_t *file, const size_t size_to_read);
const std::vector<uint64_t> read_entry_offsets_pef(const uint8_t *file, const size_t size_to_read);
const std::vector<uint64_t> read_entry_offsets_fls(const uint8_t *file, const size_t size_to_read);
const std::vector<uint64_t> read_entry_offsets_bp128(const uint8_t *file, const size_t size_to_read, uint8_t enc_type);
void check_all_pef_tokens(const Tokenizer &tokenizer,
                          const std::string &file_path);
std::pair<uint64_t, uint64_t>
get_pef_header_details_by_type(const uint8_t *file, const uint64_t file_size,
                               const uint8_t type);
void check_index_dir_valid(const std::string &index_path);

template <MapType M>
std::vector<M> read_pef_tokens(const std::string &file_path,
                               const uint64_t offset,
                               const uint64_t size_to_read) {

  if (!size_to_read) 
    return std::vector<M>();   

  const MappedFile mf = open_mmap_r(file_path);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();

  // Read in universe and n first
  const auto  &[header, size] = read_bv_header(file + offset);

  // If null return empty vec
  if (!header.n)
    return std::vector<M>();
  
  const succinct::bit_vector bv = get_bv(file + offset + size, header, size_to_read - size);

  const std::vector<uint64_t> offsets = partitioned_ef_decode_u64(bv, header.universe, header.n);

  return offsets;
}

template <MapType M>
const uint64_t create_pef_tokens(const std::vector<M> &tokens,
                                 std::ofstream &file) {
  if (!tokens.size()) {
    const uint64_t pos = file.tellp();
    return pos;
  }

  const uint64_t pos = file.tellp();

  succinct::bit_vector pef_tokens;
  const Pef_Header header = partitioned_ef_encode_u64(pef_tokens, tokens);
  const std::vector<uint8_t> compressed = gvb_encode_uint64_t({header.num_bits+1, header.universe+1, header.n+1});

  // Write size of following compressed header
  const uint8_t size_byte = static_cast<uint8_t>(compressed.size());
  file.write(reinterpret_cast<const char *>(&size_byte), sizeof(size_byte));

  // Write num_bits, universe and n
  file.write(reinterpret_cast<const char *>(compressed.data()), compressed.size());
  
  const uint8_t c_header_size = sizeof(uint8_t) + compressed.size();

  // Write to file
  const uint64_t pef_tokens_size = pef_tokens.data().size() * sizeof(uint64_t);

  // Write pef_tokens
  file.write(reinterpret_cast<const char *>(pef_tokens.data().data()),
             pef_tokens_size);

  return pos;
}

template <MapType M>
bool check_pef_tokens(const std::vector<M> &tokens, const std::string file_path,
                      const uint8_t type) {
  const MappedFile mf = open_mmap_r(file_path);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();

  const std::pair<uint64_t, uint64_t> tok_details =
      get_pef_header_details_by_type(file, file_size, type);
  const std::vector<M> out =
      read_pef_tokens<M>(file_path, tok_details.first, tok_details.second);

  if (out == tokens) {
    std::cout << "Type " << +type << " correct" << std::endl;
    return true;
  } else {
    std::cout << "Type: " << +type << " incorrect" << std::endl;
    return false;
  }
}

template <MapType M>
const Write_pos
write_offsets_to_file(const std::map<M, std::map<uint64_t, std::vector<uint64_t>>> &tokens_map,
                      std::ofstream &file, const uint8_t enc_type) {
  if (tokens_map.size() == 0) {
    return Write_pos{static_cast<uint64_t>(file.tellp()),
                     static_cast<uint64_t>(file.tellp())};
  }

  std::vector<uint64_t> id_to_offset;

  const uint64_t first_write_pos = file.tellp();

  // Offsets begin at 1 as a requirement of the gvb encoding and decoding steps
  // A value of 0 is used as terminator when decoding
  uint64_t offset = 1;
  for (const auto &[_, fields] : tokens_map) {
    id_to_offset.push_back(offset);
    
    const uint16_t num_fields = fields.size();

    std::vector<uint16_t> field_ids;
    field_ids.reserve(num_fields);
    for (const auto &[field_id, _] : fields)
      field_ids.push_back(field_id);

    std::vector<uint32_t> field_offsets;
    field_offsets.reserve(num_fields);

    // Write header with dummy value for field_offsets, will be overwritten later
    file.write(reinterpret_cast<const char*>(&num_fields), sizeof(uint16_t));
    file.write(reinterpret_cast<const char*>(field_ids.data()), num_fields * sizeof(uint16_t));

    const uint64_t field_offsets_write_pos = file.tellp();
    std::vector<uint32_t> dummy_offsets(num_fields, 0);
    file.write(reinterpret_cast<const char*>(dummy_offsets.data()), num_fields * sizeof(uint32_t));

    const uint64_t f_header_size = sizeof(uint32_t) * num_fields + sizeof(uint16_t) * num_fields + sizeof(uint16_t);

    uint64_t local_offset = offset + f_header_size;
    for (const auto &[_, offsets] : fields) {
      // Duplicate docids may be present from tokenization stage, remove them
      const std::vector<uint64_t> offsets_dedup = remove_dups_sorted(offsets);

      field_offsets.push_back(local_offset);

      switch (enc_type) {
        case EncType::GVB:
          local_offset += write_gvb_to_file(offsets_dedup, file);
          break;
        case EncType::PEF:
          local_offset += write_pef_to_file(offsets_dedup, file);
          break;
        case EncType::FLS:
          local_offset += write_fls_to_file(offsets_dedup, file);
          break;
        case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
        case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM:
          local_offset += write_bp128_to_file(offsets_dedup, file, enc_type);
          break;
        default:
          throw std::invalid_argument("Not a valid encoding type");
      }
    }

    // Replace dummy field_offsets with updated version
    std::streampos current_pos = file.tellp();
    file.seekp(field_offsets_write_pos);
    file.write(reinterpret_cast<const char*>(field_offsets.data()), num_fields * sizeof(uint32_t));
    file.seekp(current_pos);

    offset = local_offset;
  }

  // Write to file
  const uint64_t second_write_pos = file.tellp();

  switch (enc_type) {
    case EncType::GVB:
      write_gvb_to_file(id_to_offset, file);
      break;
    case EncType::PEF:
      write_pef_to_file(id_to_offset, file);
      break;
    case EncType::FLS:
      write_fls_to_file(id_to_offset, file);
      break;
    case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
    case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM:
      write_bp128_to_file(id_to_offset, file, enc_type);
      break;
    default:
      throw std::invalid_argument("Not a valid encoding type");
  }

  return Write_pos{first_write_pos, second_write_pos};
}

template <MapType M>
bool check_offsets_file_by_type(
    const std::map<M, std::map<uint64_t, std::vector<uint64_t>>> &tokens_map,
    const std::string &file_name, const uint8_t type, const uint8_t enc_type) {
  std::vector<uint64_t> id_to_offset;

  uint64_t offset = 1;
  for (const auto &[_, fields] : tokens_map) {
    id_to_offset.push_back(offset);
    
    const uint16_t num_fields = fields.size();

    std::vector<uint16_t> field_ids;
    field_ids.reserve(num_fields);
    for (const auto &[field_id, _] : fields)
      field_ids.push_back(field_id);

    std::vector<uint32_t> field_offsets;
    field_offsets.reserve(num_fields);

    const uint64_t f_header_size = sizeof(uint32_t) * num_fields + sizeof(uint16_t) * num_fields + sizeof(uint16_t);

    uint64_t local_offset = offset + f_header_size;
    for (const auto &[id, offsets] : fields) {
      // Duplicate docids may be present from tokenization stage, remove them
      const std::vector<uint64_t> offsets_dedup = remove_dups_sorted(offsets);

      field_offsets.push_back(local_offset);

      switch (enc_type) {
        case EncType::GVB:
        {
          const std::vector<uint8_t> enc = gvb_encode_uint64_t(delta_encode(offsets_dedup));
          const uint64_t enc_size = enc.size();
          local_offset += enc.size();
          break;
        }
        case EncType::PEF:
        {
          succinct::bit_vector enc;
          const Pef_Header header = partitioned_ef_encode_u64(enc, offsets_dedup);
          const std::vector<uint8_t> compressed = gvb_encode_uint64_t({header.num_bits+1, header.universe+1, header.n+1});

          // Write size of following compressed header
          const uint8_t size_byte = static_cast<uint8_t>(compressed.size());
          
          const uint8_t c_header_size = sizeof(uint8_t) + compressed.size();

          uint64_t enc_size = enc.data().size() * sizeof(uint64_t);

          local_offset += enc_size + c_header_size;
          break;
        }
        case EncType::FLS:
        {
            std::vector<uint8_t> enc_data;
            std::vector<uint8_t> enc_num_blocks = fls_encode_uint64_t(offsets_dedup, enc_data);
            const uint64_t enc_data_size = enc_data.size();
            const uint64_t enc_num_blocks_size = enc_num_blocks.size();
            local_offset += enc_data_size + enc_num_blocks_size;
          break;
        }
        case EncType::S4_BPD1: case EncType::S4_BPD2: case EncType::S4_BPD4: case EncType::S4_BPDM: 
        case EncType::S8_BPD4: case EncType::S8_BPDM: case EncType::S16_BPD4: case EncType::S16_BPDM: 
        {
          size_t orig_size = offsets_dedup.size();

          std::vector<uint32_t> vec32(offsets_dedup.begin(), offsets_dedup.end());

          for (const auto v : offsets_dedup) {
            if (v > UINT32_MAX) {
              throw std::runtime_error("uint32_t conversion overflow");
            }
          }
          const std::vector<uint32_t> enc_data = encode_simdbp128(vec32, enc_type);
          const uint64_t enc_data_size = enc_data.size() * sizeof(uint32_t);

          local_offset += enc_data_size + sizeof(orig_size);
          break;
        }
        default:
          throw std::invalid_argument("Not a valid encoding type");
      }
    }

    offset = local_offset;
  }

  // Check id_to_offsets
  std::cout << "here1" << std::endl;
  std::cout << id_to_offset.size() << std::endl;
  std::vector<uint64_t> id_to_offset_from_file = get_id_offsets_from_file(file_name, type, enc_type);
  std::cout << "here2" << std::endl;
  if (id_to_offset_from_file != id_to_offset) {
    std::cout << "id offsets incorrect\n";
    std::cout << "file\n";
    for (auto off : id_to_offset_from_file) {
      std::cout << off << std::endl;
    }

    std::cout << "og\n";
    for (auto off : id_to_offset) {
      std::cout << off << std::endl;
    }
    return false;
  }


  uint64_t token_id = 0;
  for (const auto &[_, fields] : tokens_map) {
    for (const auto &[field_id, docids] : fields) {
      const std::vector<uint64_t> docids_dedup = remove_dups_sorted(docids); 
      std::vector<uint64_t> from_file = get_tok_offsets_from_file(token_id, id_to_offset, std::vector<uint64_t>() = {field_id}, file_name, type, enc_type);
      for (auto &i : from_file) ++i; // Normalise
      if (from_file != docids_dedup) {
        std::cout << "from_file size: " << from_file.size() << std::endl;
        std::cout << "docids size: " << docids.size() << std::endl;
        std::cout << "docids incorrect" << std::endl;
        return false;
      }
    }
    ++token_id;
  }

  std::cout << "correct" << std::endl;
  return true;
}

#endif