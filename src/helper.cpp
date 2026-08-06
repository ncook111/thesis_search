#include "helper.h"
#include "tokenizer.h"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

using namespace boost::interprocess;

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

  throw std::invalid_argument("Encoding type not supported");
}

const std::string enc_type_to_str(const uint8_t &type) {
  if (type == EncType::GVB)
    return "gvb";
  if (type == EncType::PEF)
    return "pef";

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
  file_mapping file_mapping(file_path.data(), read_only);
  mapped_region file_region(file_mapping, read_only);

  return MappedFile{std::move(file_mapping), std::move(file_region)};
}

const MappedFile open_mmap_rw(const std::string &file_path) {
  file_mapping file_mapping(file_path.data(), read_write);
  mapped_region file_region(file_mapping, read_write);

  return MappedFile{std::move(file_mapping), std::move(file_region)};
}

const std::string token_type_to_str(const uint8_t &type) {
  switch (type) {
  case TokenType::ALPHA:
    return "alpha";
  case TokenType::INT:
    return "int";
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
    return TokenType::INT;
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