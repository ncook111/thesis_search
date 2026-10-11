#ifndef HELPER_H
#define HELPER_H

#include "custom_types.h"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <map>
#include <vector>

struct MappedFile {
  boost::interprocess::file_mapping mapping;
  boost::interprocess::mapped_region region;

  [[nodiscard]] void *addr() const { return region.get_address(); }
  [[nodiscard]] std::size_t size() const { return region.get_size(); }
};

namespace FileType {
enum Value : uint8_t { CSV = 0, JSON = 1, LOGFMT = 2, INVALID = 3 };
}

namespace FstType {
enum Value : uint8_t { ALPHA = 0, FLOAT = 1, FIELDS = 2, OTHER = 3, INVALID = 4 };
}

namespace EncType {
enum Value : uint8_t { 
  GVB = 0,
  PEF = 1,
  FLS = 2,
  S4_BPD1 = 3,
  S4_BPD2 = 4,
  S4_BPD4 = 5,
  S4_BPDM = 6,
  S8_BPD4 = 7,
  S8_BPDM = 8,
  S16_BPD4 = 9,
  S16_BPDM = 10,
  INVALID = 11 };
}

namespace QueryMode {
enum Value : uint8_t { UNION = 0, INTER = 1, COMP = 2, INVALID = 3 };
}

const bool pef_or_fst(const uint8_t type);
const bool valid_extension(const std::string &ext);
const uint8_t get_file_type(const std::string &type);
const uint8_t get_enc_type(const std::string &type);
const std::string enc_type_to_str(const uint8_t &type);
const MappedFile open_mmap_r(const std::string &file_path);
const MappedFile open_mmap_rw(const std::string &file_path);
const std::string token_type_to_str(const uint8_t &type);
const uint8_t str_to_token_type(const std::string &type_str);
const std::vector<uint32_t> remove_dups_sorted(const std::vector<uint32_t> &vec);
const std::vector<uint32_t> remove_dups_unsorted(std::vector<uint32_t> &vec);
const uint8_t get_query_mode(const std::string &mode);
const std::string trim(const std::string& str);
const uint32_t ip_str_to_int(const int domain, const std::string ip_addr, bool &fail);
bool is_digit(const char c);
bool is_alpha(const char c);
bool is_alnum(const char c);
char to_lower(const char c);

template <class F> const std::vector<uint8_t> to_bytes(const F &input) {
  std::vector<uint8_t> bytes(sizeof(input));
  std::memcpy(bytes.data(), &input, sizeof(input));

  return bytes;
}

template <class F> const F from_bytes(const std::vector<uint8_t> &bytes) {
  F val;
  std::memcpy(&val, bytes.data(), sizeof(F));
  return val;
}

template <FstQueryType Q, MapType M, class T>
const std::vector<Q> tokens_from_map(const std::map<M, T> &tokens_map) {
  std::vector<Q> tokens;

  // Handle strings differently to all other M types
  if constexpr (std::is_same_v<M, std::string>) {
    for (auto &tok : tokens_map) {
      tokens.push_back(tok.first);
    }
  } else {
    for (auto &tok : tokens_map) {
      tokens.push_back(to_bytes(tok.first));
    }
  }

  return tokens;
}

#endif