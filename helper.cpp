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
  if (type == "-gvb")
    return EncType::GVB;
  if (type == "-pef")
    return EncType::PEF;

  throw std::invalid_argument("Encoding type not supported");
}

const uint8_t get_query_mode(const std::string &mode) {
  if (mode == "-u")
    return QueryMode::UNION;
  if (mode == "-i")
    return QueryMode::INTER;

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

const std::vector<uint64_t> remove_dups(const std::vector<uint64_t> &vec) {
  std::vector<uint64_t> dedup;
  for (const auto &v : vec) {
    if (dedup.size() && dedup.back() == v)
      continue;
    dedup.push_back(v);
  }

  return dedup;
}