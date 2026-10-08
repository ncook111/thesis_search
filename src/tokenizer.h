#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "date/date.h"
#include "fast_float/fast_float.h"
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <time.h>

constexpr uint32_t MAX_TOK_LENGTH = 256;

namespace TokenType {
  enum Value : uint8_t {
    ALPHA = 0,
    PINT = 1,
    NINT = 2,
    FLOAT = 3,
    DATE = 4,
    TIME = 5,
    IPV4 = 6,
    IPV6 = 7,
    BOOLEAN = 8,
    INVALID = 9,
  };
}

class Tokenizer {
public:
  static Tokenizer from_file(const std::string &file_path, 
    const uint8_t type, const std::vector<std::string> &fields);
  static Tokenizer from_string(const std::string &input);
  void close();
  
  // A map containing an entry for each token
  // E.g. "Wooden Table" would have an entry for "wooden" and "table"
  // Each entry contains a map for each field tokenized, with each vector containing
  // Another vector of docids in which that token is found
  std::map<std::string, std::map<uint64_t, std::vector<uint64_t>>> alpha_tokens;
  std::map<uint64_t, std::map<uint64_t, std::vector<uint64_t>>> num_tokens;
  std::map<uint64_t, std::map<uint64_t, std::vector<uint64_t>>> neg_num_tokens;
  std::map<float, std::map<uint64_t, std::vector<uint64_t>>> float_tokens;
  std::map<uint64_t, std::map<uint64_t, std::vector<uint64_t>>> d_tokens;
  std::map<uint64_t, std::map<uint64_t, std::vector<uint64_t>>> t_tokens;
  std::map<uint64_t, std::map<uint64_t, std::vector<uint64_t>>> ipv4_tokens;
  std::map<uint64_t, std::map<uint64_t,std::vector<uint64_t>>> ipv6_tokens;
  
  // Map of boolean tokens for each field
  std::map<uint64_t, std::vector<uint64_t>> t_bool_tokens;
  std::map<uint64_t, std::vector<uint64_t>> f_bool_tokens;
  
  // Denotes the start point file offset for all entries in the file being tokenized
  std::vector<uint64_t> entry_offsets; 

  // Used when tokenizing a query string
  std::set<std::string> alpha_tokens_q;
  std::set<uint64_t> num_tokens_q;
  std::set<uint64_t> neg_num_tokens_q;
  std::set<float> float_tokens_q;
  std::set<uint64_t> d_tokens_q;
  std::set<uint64_t> t_tokens_q;
  std::set<uint64_t> ipv4_tokens_q;
  std::set<uint64_t> ipv6_tokens_q;
  bool t_bool_token_q = false;
  bool f_bool_token_q = false;

  std::vector<std::string> fields;

  void printAlphaTokens() const;
  void printNumTokens() const;
  void printNegNumTokens() const;
  void printFloatTokens() const;
  void printDateTokens() const;
  void printTimeTokens() const;
  void printIPTokens() const;
  void printStatistics() const;

  const std::vector<std::string> getAlphaTokens() const;
  const std::vector<uint64_t> getNumTokens() const;
  const std::vector<uint64_t> getNegNumTokens() const;
  const std::vector<float> getFloatTokens() const;
  const std::vector<uint64_t> getDateTokens() const;
  const std::vector<uint64_t> getTimeTokens() const;
  const std::vector<uint64_t> getIPTokens() const;
  const std::vector<std::string> getAlphaTokensQ() const;
  const std::vector<uint64_t> getNumTokensQ() const;
  const std::vector<uint64_t> getNegNumTokensQ() const;
  const std::vector<float> getFloatTokensQ() const;
  const std::vector<uint64_t> getDateTokensQ() const;
  const std::vector<uint64_t> getTimeTokensQ() const;
  const std::vector<uint64_t> getIPTokensQ() const;

private:
  Tokenizer() = default;
  void tokenize_file();
  void tokenize_string(const std::string &input);
  void processToken(std::string &curr_token, const uint64_t doc_id, const uint64_t field_id);
  void processToken(std::string &curr_token);
  void tokenize_csv();
  void tokenize_json();
  void tokenize_logfmt();
  void tokenize_field(const std::string_view field_str, const uint64_t doc_id, const uint64_t field_id);
  const bool is_ipv4(const std::string &s) const;
  const bool is_time(const std::string &s) const;
  const bool is_date(const std::string &s) const;
  const bool is_float(const std::string &s) const;
  const bool is_pint(const std::string &s) const;
  const bool is_nint(const std::string &s) const;
  const bool is_bool(const std::string &s) const;
  const bool contains_alpha(const std::string &s) const;
  bool handle_pint(std::string &curr_token, const uint64_t offset = 0, const uint64_t field_id = 0);
  bool handle_nint(std::string &curr_token, const uint64_t offset = 0, const uint64_t field_id = 0);
  bool handle_float(std::string &curr_token, const uint64_t offset = 0, const uint64_t field_id = 0);
  bool handle_date(std::string &curr_token, const uint64_t offset = 0, const uint64_t field_id = 0);
  bool handle_time(std::string &curr_token, const uint64_t offset = 0, const uint64_t field_id = 0);
  bool handle_ipv4(std::string &curr_token, const uint64_t offset = 0, const uint64_t field_id = 0);
  bool handle_bool(std::string &curr_token, const uint64_t offset = 0, const uint64_t field_id = 0);
  void handle_alpha(std::string &curr_token, const uint64_t offset = 0, const uint64_t field_id = 0);
  inline void vector_inserter(std::vector<uint64_t> &vec, const uint64_t value);

  std::string file_path;
  uint8_t type;
};

#endif