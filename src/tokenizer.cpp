#include "tokenizer.h"
#include "coder.h"
#include "csv.hpp"
#include "date/date.h"
#include "fast_float/fast_float.h"
#include "helper.h"
#include "ingest.h"
#include "simdjson.h"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <time.h>

const bool Tokenizer::is_int(const std::string &s) const {
  size_t i = s.front() == '+' || s.front() == '-' ? 1 : 0;

  if (i == s.size())
    return false;

  for (; i < s.size(); ++i) {
    if (!std::isdigit(s[i]))
      return false;
  }

  return true;
}

const bool Tokenizer::is_float(const std::string &s) const {
  // Preprocess string for float compliance
  for (const auto c : s) {
    if (!std::isdigit(c) && c != '+' && c != '-' && c != '.') {
      return false;
    }
  }

  // Determine if float
  try {
    size_t processed; // Number of characters processed, must match string size
                      // to be valid float
    std::stof(s, &processed);

    if (processed == s.size())
      return true;
  } catch (const std::invalid_argument) {
    return false;
  } catch (const std::out_of_range) {
    return false;
  }

  return false;
}

const bool Tokenizer::is_date(const std::string &s) const {
  if (s.size() != 10)
    return false;
  if (!std::isdigit(s.at(0)))
    return false;
  if (!std::isdigit(s.at(1)))
    return false;
  if (!std::isdigit(s.at(2)))
    return false;
  if (!std::isdigit(s.at(3)))
    return false;
  if (s.at(4) != '-')
    return false;
  if (!std::isdigit(s.at(5)))
    return false;
  if (!std::isdigit(s.at(6)))
    return false;
  if (s.at(7) != '-')
    return false;
  if (!std::isdigit(s.at(8)))
    return false;
  if (!std::isdigit(s.at(9)))
    return false;
  // Must be YYYY-MM-DD format
  return true;
}

const bool Tokenizer::is_time(const std::string &s) const {
  // Determine if time present
  if (s.size() != 8)
    return false;
  if (!std::isdigit(s.at(0)))
    return false;
  if (!std::isdigit(s.at(1)))
    return false;
  if (s.at(2) != ':')
    return false;
  if (!std::isdigit(s.at(3)))
    return false;
  if (!std::isdigit(s.at(4)))
    return false;
  if (s.at(5) != ':')
    return false;
  if (!std::isdigit(s.at(6)))
    return false;
  if (!std::isdigit(s.at(7)))
    return false;

  return true;
}

const bool Tokenizer::is_ipv4(const std::string &s) const {
  if (s.size() < 7 || s.size() > 15)
    return false; // Max length of ipv4

  // Check only valid chars
  for (const auto c : s) {
    if (std::isdigit(c) || c == '.')
      continue;
    else
      return false;
  }

  // Check valid format
  std::vector<std::string> parts;
  std::string s1 = s;
  size_t pos = s.find('.');
  while (pos != std::string::npos) {
    parts.emplace_back(s1.substr(0, pos));
    s1.erase(0, pos + 1);
    pos = s1.find('.');
  }
  parts.emplace_back(s1.substr(0, pos));

  if (parts.size() != 4)
    return false;

  for (const auto curr : parts) {
    std::uint64_t res_int;
    fast_float::from_chars_result res_err =
        fast_float::from_chars(curr.data(), curr.data() + curr.size(), res_int);
    if (res_err.ec == std::errc()) {
      if (res_int > 256)
        return false;
    } else if (res_err.ec == std::errc::invalid_argument) {
      return false;
    } else if (res_err.ec == std::errc::result_out_of_range) {
      return false;
    }
  }

  return true;
}

const bool Tokenizer::is_ipv6(const std::string &s) const { return false; }

const bool Tokenizer::is_bool(const std::string &s) const {
  return (s == "true" || s == "false");
}

inline const uint8_t Tokenizer::numeric_type(const std::string &s) const {
  if (is_int(s)) {
    return TokenType::INT;
  }
  if (is_float(s)) {
    return TokenType::FLOAT;
  }
  if (is_date(s)) {
    return TokenType::DATE;
  }
  if (is_time(s)) {
    return TokenType::TIME;
  }
  if (is_ipv4(s)) {
    return TokenType::IPV4;
  }
  if (is_bool(s)) {
    return TokenType::BOOLEAN;
  }
  return TokenType::ALPHA;
}

void Tokenizer::tokenize_file() {
  switch (type) {
  case FileType::CSV:
    tokenize_csv();
    break;
  case FileType::JSON:
    tokenize_json();
    break;
  case FileType::LOGFMT:
    tokenize_logfmt();
    break;
  default:
    break;
  }
}

void Tokenizer::tokenize_csv() {
  std::uint64_t file_size = std::filesystem::file_size(std::filesystem::path(file_path));

  csv::CSVFormat format;
  format.variable_columns(csv::VariableColumnPolicy::THROW);
  csv::CSVReader reader(file_path, format);

  if (fields.empty())
    fields = reader.get_col_names();

  for (const csv::CSVRow &row : reader) { // Input iterator
    entry_offsets.push_back(row.byte_offset() + 1);

    for (uint64_t field_id = 0; field_id < fields.size(); ++field_id )
      tokenize_field(row[fields[field_id]].get<std::string_view>(), entry_offsets.size(), field_id);

    if (entry_offsets.size() % 10000 == 0)
      std::cout << "Processed: "
                << (float)(row.byte_offset() / (float)file_size) * 100 << "%"
                << std::endl;
  }

  entry_offsets.push_back(file_size + 2);
  std::cout << "Lines: " << entry_offsets.size() << std::endl;
}

void Tokenizer::tokenize_json() {
  std::uint64_t file_size = std::filesystem::file_size(std::filesystem::path(file_path));

  using namespace simdjson;

  ondemand::parser parser;
  simdjson::ondemand::document_stream stream;
  padded_string json = padded_string::load(file_path);
  auto error = parser.iterate_many(json).get(stream);

  uint64_t doc_id = 1; // Doc_id's 1-indexed
  for (auto j = stream.begin(); j != stream.end(); ++j) {
    auto doc = *j;
    ondemand::object obj;

    error = doc.get_object().get(obj);
    if (error) throw std::invalid_argument("JSON deserialisation error: " + std::to_string(error));

    // If no fields add all fields
    // This assumes fields for every json object will be the same
    if (!fields.size()) {
      for (auto field : obj) {
        std::string_view f = field.escaped_key();
        fields.push_back(std::string(f));
      }
    }

    for (uint64_t field_id = 0; field_id < fields.size(); ++field_id) {
      std::string_view text;
      if (obj[fields[field_id]].value().get(text) == 0) 
        tokenize_field(text, doc_id, field_id);
    }

    entry_offsets.push_back(j.current_index() + 1);

    if (doc_id % 10000 == 0)
      std::cout << "Processed: "
                << (float)(j.current_index() / (float)file_size) * 100 << "%"
                << std::endl;

    ++doc_id;
  }

  entry_offsets.push_back(file_size + 1);

  std::cout << "Lines: " << entry_offsets.size() << std::endl;
}

void Tokenizer::tokenize_logfmt() {}

void Tokenizer::tokenize_field(const std::string_view field_str,
                               const uint64_t doc_id, const uint64_t field_id) {
  std::string curr_token;
  for (const char c : field_str) {
    if (isalnum(c) || c == '+' || c == '-' || c == '.' || c == ':')
      curr_token.push_back(c);
    else if (curr_token.size())
        processToken(curr_token, doc_id, field_id);
  }

  // Process leftover token if present
  if (curr_token.size())
    processToken(curr_token, doc_id, field_id);
}

void Tokenizer::tokenize_string(const std::string &input) {
  std::string curr_token;
  const size_t input_size = input.size();

  for (const char c : input) {
    if (isalnum(c) || c == '+' || c == '-' || c == '.' || c == ':')
      curr_token.push_back(c);
    else if (curr_token.size())
      processToken(curr_token);
  }

  // Process leftover token if required
  if (curr_token.size())
    processToken(curr_token);
}

void Tokenizer::handle_int(std::string &curr_token, const uint64_t offset, const uint64_t field_id) {
  std::int64_t res_int;
  fast_float::from_chars_result res_err;
  fast_float::parse_options options{fast_float::chars_format::allow_leading_plus};

  res_err = fast_float::from_chars_advanced(curr_token.data(), curr_token.data() + curr_token.size(), res_int, options);
  if (res_err.ec == std::errc()) {
    // File tokenization
    if (offset) {
      if (res_int >= 0)
        num_tokens[res_int][field_id].push_back(offset);
      else
        num_tokens[res_int * -1][field_id].push_back(offset);
    // String tokenization
    } else {
      if (res_int >= 0)
        num_tokens_q.insert(res_int);
      else
        num_tokens_q.insert(res_int * -1);
    }
  } else if (res_err.ec == std::errc::invalid_argument) {
    std::cout << "Malformed number: \"" << curr_token << "\"" << std::endl;
  } else if (res_err.ec == std::errc::result_out_of_range) {
    return;
  }

  curr_token.erase();
}

void Tokenizer::handle_float(std::string &curr_token, const uint64_t offset, const uint64_t field_id) {
  float res_float;
  fast_float::from_chars_result res_err;
  fast_float::parse_options options{fast_float::chars_format::allow_leading_plus};
  res_err = fast_float::from_chars_advanced(curr_token.data(), curr_token.data() + curr_token.size(), res_float, options);
  if (res_err.ec == std::errc()) {
    // File tokenization
    if (offset)
      float_tokens[std::move(res_float)][field_id].push_back(offset);
    // String tokenization
    else
      float_tokens_q.insert(res_float);
  } else if (res_err.ec == std::errc::invalid_argument) {
    std::cout << "Malformed number: \"" << curr_token << "\"" << std::endl;
  } else if (res_err.ec == std::errc::result_out_of_range) {
    return;
  }

  curr_token.erase();
}

void Tokenizer::handle_date(std::string &curr_token, const uint64_t offset, const uint64_t field_id) {
  std::istringstream in(curr_token);
  date::sys_time<std::chrono::seconds> dateTimePoint;

  // Check "YY-MM-DD format"
  in >> date::parse("%Y-%m-%d", dateTimePoint);
  // Check if parsable
  if (in.fail() || in.peek() != EOF) {
    return;
  } else {
    // File tokenization
    if (offset)
      d_tokens[static_cast<uint64_t>(dateTimePoint.time_since_epoch().count())][field_id].push_back(offset);
    // String tokenization
    else
      d_tokens_q.insert(static_cast<uint64_t>(dateTimePoint.time_since_epoch().count()));
  }

  curr_token.erase();
}
void Tokenizer::handle_time(std::string &curr_token, const uint64_t offset, const uint64_t field_id) {
  std::istringstream in(curr_token);
  std::chrono::seconds timeTimePoint;

  // Check "HH-MM-SS format"
  in >> date::parse("%T", timeTimePoint);
  // Check if parsable
  if (in.fail() || in.peek() != EOF) {
    return;
  } else {
    // File tokenization
    if (offset)
      t_tokens[static_cast<uint64_t>(timeTimePoint.count())][field_id].push_back(offset);
    // String tokenization
    else
      t_tokens_q.insert(static_cast<uint64_t>(timeTimePoint.count()));
  }

  curr_token.erase();
}
void Tokenizer::handle_ipv4(std::string &curr_token, const uint64_t offset, const uint64_t field_id) {
  // File tokenization
  if (offset)
    ipv4_tokens[1][field_id].push_back(offset);
  // String tokenization
  else
    ipv4_tokens_q.insert(1);
  
  curr_token.erase();
}

void Tokenizer::handle_ipv6(std::string  &curr_token, const uint64_t offset, const uint64_t field_id) {
  // File tokenization
  if (offset)
    ipv6_tokens[1][field_id].push_back(offset);
  // String tokenization
  else
    ipv6_tokens_q.insert(1);
  
  curr_token.erase();
}

void Tokenizer::handle_bool(std::string  &curr_token, const uint64_t offset, const uint64_t field_id) {
  // File tokenization
  if (offset) {
    if (curr_token == "true")
      t_bool_tokens[field_id].push_back(offset);
    if (curr_token == "false")
      f_bool_tokens[field_id].push_back(offset);
  // String tokenization
  } else {
    if (curr_token == "true")
      t_bool_token_q = true;
    if (curr_token == "false")
      f_bool_token_q = true;
  }

  curr_token.erase();
};


void Tokenizer::processToken(std::string &curr_token, const uint64_t offset, const uint64_t field_id) {
  switch (numeric_type(curr_token)) {
    case TokenType::INT:
      handle_int(curr_token, offset, field_id);
      break;
    case TokenType::FLOAT:
      handle_float(curr_token, offset, field_id);
      break;
    case TokenType::DATE:
      handle_date(curr_token, offset, field_id);
      break;
    case TokenType::TIME:
      handle_time(curr_token, offset, field_id);
      break;
    case TokenType::IPV4:
      handle_ipv4(curr_token, offset, field_id);
      break;
    case TokenType::IPV6:
      handle_ipv6(curr_token, offset, field_id);
      break;
    case TokenType::BOOLEAN:
      handle_bool(curr_token, offset, field_id);
      break;
    default:
      break;
  }

  if (curr_token.size()) {
    std::ranges::transform(curr_token.begin(), curr_token.end(), curr_token.begin(), [](unsigned char c) { return std::tolower(c); });
    // if (curr_token == "high" || curr_token == "school") {
    //   std::cout << curr_token;
    //   std::cout << " field id: " << field_id << " docid: " << offset << std::endl;
    // }
    alpha_tokens[std::move(curr_token)][field_id].push_back(offset);
    curr_token.erase();
  }
}

void Tokenizer::processToken(std::string &curr_token) {
  std::cout << curr_token << std::endl;
  switch (numeric_type(curr_token)) {
    case TokenType::INT:
      handle_int(curr_token);
      break;
    case TokenType::FLOAT:
      handle_float(curr_token);
      break;
    case TokenType::DATE:
      handle_date(curr_token);
      break;
    case TokenType::TIME:
      handle_time(curr_token);
      break;
    case TokenType::IPV4:
      handle_ipv4(curr_token);
      break;
    case TokenType::IPV6:
      handle_ipv6(curr_token);
      break;
    case TokenType::BOOLEAN:
      handle_bool(curr_token);
      break;
    default:
      break;
  }

  if (curr_token.size()) {
    std::ranges::transform(curr_token.begin(), curr_token.end(), curr_token.begin(), [](unsigned char c) { return std::tolower(c); });
    alpha_tokens_q.insert(std::move(curr_token));
    curr_token.erase();
  }
}

void Tokenizer::printAlphaTokens() const {
  for (const auto &[key, _] : alpha_tokens)
    std::cout << key << std::endl;
}

void Tokenizer::printNumTokens() const {
  for (const auto &[key, _] : num_tokens)
    std::cout << key << std::endl;
}

void Tokenizer::printFloatTokens() const {
  for (const auto &[key, _] : float_tokens)
    std::cout << key << std::endl;
}

void Tokenizer::printDateTokens() const {
  for (const auto &[key, _] : d_tokens) {
    time_t tm = key;
    std::tm *tm_info = std::gmtime(&tm);
    std::ostringstream oss;
    oss << std::put_time(tm_info, "%Y-%m-%d");
    std::cout << oss.str() << std::endl;
  }
}

void Tokenizer::printTimeTokens() const {
  for (const auto &[key, _] : t_tokens) {
    time_t tm = key;
    std::tm *tm_info = std::gmtime(&tm);
    std::ostringstream oss;
    oss << std::put_time(tm_info, "%T");
    std::cout << oss.str() << std::endl;
  }
}

void Tokenizer::printIPTokens() const {}

Tokenizer Tokenizer::from_file(const std::string &file_path, const uint8_t type,
                               const std::vector<std::string> &fields) {
  Tokenizer t;
  t.file_path = file_path;
  t.fields = fields;
  t.type = type;
  t.tokenize_file();

  return t;
}

Tokenizer Tokenizer::from_string(const std::string &file_path) {
  Tokenizer t;
  t.tokenize_string(file_path);

  return t;
}

void Tokenizer::printStatistics() const {
  std::cout << "Total Alpha Tokens: " << alpha_tokens.size() << std::endl;
  std::cout << "Total Integer Tokens: " << num_tokens.size() << std::endl;
  std::cout << "Total Float Tokens: " << float_tokens.size() << std::endl;
  std::cout << "Total Date Tokens: " << d_tokens.size() << std::endl;
  std::cout << "Total Time Tokens: " << t_tokens.size() << std::endl;
  std::cout << "Total Ipv4 Tokens: " << ipv4_tokens.size() << std::endl;
  std::cout << "Total Ipv6 Tokens: " << ipv6_tokens.size() << std::endl;
  std::cout << "Total True Boolean Tokens: " << t_bool_tokens.size() << std::endl;
  std::cout << "Total False Boolean Tokens: " << f_bool_tokens.size() << std::endl;
}

const std::vector<std::string> Tokenizer::getAlphaTokens() const {
  std::vector<std::string> tokens;
  for (const auto &i : alpha_tokens)
    tokens.push_back(i.first);

  return tokens;
}

const std::vector<uint64_t> Tokenizer::getNumTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : num_tokens)
    tokens.push_back(i.first);

  return tokens;
}
const std::vector<float> Tokenizer::getFloatTokens() const {
  std::vector<float> tokens;
  for (const auto i : float_tokens)
    tokens.push_back(i.first);

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getDateTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : d_tokens)
    tokens.push_back(i.first);

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getTimeTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : t_tokens)
    tokens.push_back(i.first);

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getIPTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : ipv4_tokens)
    tokens.push_back(i.first);

  return tokens;
}

const std::vector<std::string> Tokenizer::getAlphaTokensQ() const {
  std::vector<std::string> tokens;
  for (const auto &i : alpha_tokens_q)
    tokens.push_back(i);

  return tokens;
}

const std::vector<uint64_t> Tokenizer::getNumTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : num_tokens_q)
    tokens.push_back(i);

  return tokens;
}
const std::vector<float> Tokenizer::getFloatTokensQ() const {
  std::vector<float> tokens;
  for (const auto i : float_tokens_q)
    tokens.push_back(i);

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getDateTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : d_tokens_q)
    tokens.push_back(i);

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getTimeTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : t_tokens_q)
    tokens.push_back(i);

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getIPTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : ipv4_tokens_q)
    tokens.push_back(i);

  return tokens;
}