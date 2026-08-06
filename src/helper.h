#ifndef HELPER_H
#define HELPER_H

#include "custom_types.h"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <map>
#include <vector>

using namespace boost::interprocess;

struct MappedFile {
  file_mapping mapping;
  mapped_region region;

  [[nodiscard]] void *addr() const { return region.get_address(); }
  [[nodiscard]] std::size_t size() const { return region.get_size(); }
};

namespace FileType {
enum Value : uint8_t { CSV = 0, JSON = 1, LOGFMT = 2, INVALID = 3 };
}

namespace EncType {
enum Value : uint8_t { GVB = 0, PEF = 1, INVALID = 2 };
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
const std::vector<uint64_t> remove_dups_sorted(const std::vector<uint64_t> &vec);
const std::vector<uint64_t> remove_dups_unsorted(std::vector<uint64_t> &vec);
const uint8_t get_query_mode(const std::string &mode);
const std::string trim(const std::string& str);

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