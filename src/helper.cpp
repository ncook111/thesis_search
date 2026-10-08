#include "helper.h"
#include "tokenizer.h"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <filesystem>
#include <arpa/inet.h>

const bool valid_extension(const std::string &ext) {
  return (ext == ".csv" || ext == ".json" || ext == ".logfmt");
}

const uint8_t get_file_type(const std::string &type) {
  if (type == ".csv")
    return 0;
  if (type == ".json")
    return 1;
  if (type == ".logfmt")
    return 2;

  throw std::invalid_argument("File type not supported");
}

const uint8_t get_enc_type(const std::string &type) {
  if (type == "gvb")
    return EncType::GVB;
  if (type == "pef")
    return EncType::PEF;
  if (type == "fls")
    return EncType::FLS;
  if (type == "s4_bpd1")
    return EncType::S4_BPD1;
  if (type == "s4_bpd2")
    return EncType::S4_BPD2;
  if (type == "s4_bpd4")
    return EncType::S4_BPD4;
  if (type == "s4_bpdm")
    return EncType::S4_BPDM;
  if (type == "s8_bpd4")
    return EncType::S8_BPD4;
  if (type == "s8_bpdm")
    return EncType::S8_BPDM;
  if (type == "s16_bpd4")
    return EncType::S16_BPD4;
  if (type == "s16_bpdm")
    return EncType::S16_BPDM;
  throw std::invalid_argument("Encoding type not supported");
}

const std::string enc_type_to_str(const uint8_t &type) {
  if (type == EncType::GVB)
    return "gvb";
  if (type == EncType::PEF)
    return "pef";
  if (type == EncType::FLS)
    return "fls";
  if (type == EncType::S4_BPD1)
    return "s4_bpd1";
  if (type == EncType::S4_BPD2)
    return "s4_bpd2";
  if (type == EncType::S4_BPD4)
    return "s4_bpd4";
  if (type == EncType::S4_BPDM)
    return "s4_bpdm";
  if (type == EncType::S8_BPD4)
    return "s8_bpd4";
  if (type == EncType::S8_BPDM)
    return "s8_bpdm";
  if (type == EncType::S16_BPD4)
    return "s16_bpd4";
  if (type == EncType::S16_BPDM)
    return "s16_bpdm";
  throw std::invalid_argument("Encoding type not supported");
}

const uint8_t get_query_mode(const std::string &mode) {
  if (mode == "u" || mode == "union")
    return QueryMode::UNION;
  if (mode == "i" || mode == "intersection")
    return QueryMode::INTER;
  if (mode == "c" || mode == "compliment")
    return QueryMode::COMP;

  throw std::invalid_argument("Query type not supported");
}

const MappedFile open_mmap_r(const std::string &file_path) {
  if (std::filesystem::file_size(file_path) == 0) {
    return MappedFile{}; // empty state, no mapping needed
  }

  boost::interprocess::file_mapping file_mapping(file_path.data(), boost::interprocess::read_only);
  boost::interprocess::mapped_region file_region(file_mapping, boost::interprocess::read_only);

  return MappedFile{std::move(file_mapping), std::move(file_region)};
}

const MappedFile open_mmap_rw(const std::string &file_path) {
  if (std::filesystem::file_size(file_path) == 0) {
    return MappedFile{}; // empty state, no mapping needed
  }

  boost::interprocess::file_mapping file_mapping(file_path.data(), boost::interprocess::read_write);
  boost::interprocess::mapped_region file_region(file_mapping, boost::interprocess::read_write);

  return MappedFile{std::move(file_mapping), std::move(file_region)};
}

const std::string token_type_to_str(const uint8_t &type) {
  switch (type) {
  case TokenType::ALPHA:
    return "alpha";
  case TokenType::PINT:
    return "int";
  case TokenType::NINT:
    return "neg_int";
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

const uint8_t str_to_token_type(const std::string &type_str) {
  if (type_str == "alpha")
    return TokenType::ALPHA;
  if (type_str == "int")
    return TokenType::PINT;
  if (type_str == "neg_int")
    return TokenType::NINT;
  if (type_str == "float")
    return TokenType::FLOAT;
  if (type_str == "date")
    return TokenType::DATE;
  if (type_str == "time")
    return TokenType::TIME;
  if (type_str == "ipv4")
    return TokenType::IPV4;

  return TokenType::INVALID;
}

const std::vector<uint64_t> remove_dups_sorted(const std::vector<uint64_t> &vec) {
  std::vector<uint64_t> dedup;
  for (const auto &v : vec) {
    if (dedup.size() && dedup.back() == v)
      continue;
    dedup.push_back(v);
  }

  return dedup;
}

const std::vector<uint64_t> remove_dups_unsorted(std::vector<uint64_t> &vec) {
  // Sort before removing dups
  std::sort(vec.begin(), vec.end());
  return remove_dups_sorted(vec);
}

const std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return ""; // The entire string is whitespace
    }

    size_t end = str.find_last_not_of(whitespace);
    
    return str.substr(start, end - start + 1);
}

const uint32_t ip_str_to_int(const int domain, const std::string ip_addr, bool &fail) {
  struct in_addr addr;
  int s;

  s = inet_pton(domain, ip_addr.data(), &addr);

  fail = (s <= 0);


  return ntohl(addr.s_addr);
}

bool is_digit(const char c) { 
  return c >= '0' && c <= '9'; 
}
bool is_alpha(const char c) { 
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_alnum(const char c) {
  return is_digit(c) || is_alpha(c);
}

char to_lower(const char c) {
  return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}