#ifndef HELPER_H
#define HELPER_H

#include "custom_types.h"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <map>
#include <vector>
#include <algorithm>

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

bool is_blank(const std::string &str);
const bool pef_or_fst(const uint8_t type);
const bool valid_extension(const std::string &ext);
const uint8_t str_to_file_type(const std::string &file_name);
const uint8_t str_to_enc_type(const std::string &type);
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

template <FstQueryType Q, class Key, class Value>
const std::vector<Q> tokens_from_map(const std::vector<std::pair<Key, const Value *>> &tokens_map) {
  std::vector<Q> tokens;

  // Handle strings differently to all other Key types
  if constexpr (std::is_same_v<Key, std::string_view>) {
    for (const auto &[tok, _] : tokens_map) {
      tokens.emplace_back(tok);
    }
  } else {
    for (const auto &[tok, _] : tokens_map) {
      tokens.push_back(to_bytes(tok));
    }
  }

  return tokens;
}

template <class T>
std::vector<T> cvrt_1(std::vector<T> vec) {
  for (auto& x : vec) ++x;
  return vec;  // implicitly moved, no extra copy
}

template <class T>
std::vector<T> cvrt_0(std::vector<T> vec) {
  for (auto& x : vec) --x;
  return vec;  // implicitly moved, no extra copy
}

template <class K, class Value,
          class Key = std::conditional_t<std::is_same_v<K, std::string>, std::string_view, K>>
std::vector<std::pair<Key, const Value *>> sorted_view(const boost::unordered_flat_map<K, Value> &m) {
  std::vector<std::pair<Key, const Value *>> out;

  out.reserve(m.size());
  for (const auto &[key, value] : m)
    out.emplace_back(key, &value);

  std::sort(out.begin(), out.end(),
    [](const std::pair<Key, const Value *> &a, const std::pair<Key, const Value *> &b) {
      return a.first < b.first;
    });

  return out;
}

#endif