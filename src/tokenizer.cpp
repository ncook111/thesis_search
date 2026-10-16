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
#include <filesystem>
#include "logfmt_parser.h"

const bool Tokenizer::contains_alpha(const std::string &s) const {
  for (size_t i = 0; i < s.size(); ++i) {
    if (is_alpha(s[i])) return true;
  }

  return false;
}
const bool Tokenizer::is_pint(const std::string &s) const {
  size_t i = s.front() == '+' ? 1 : 0;

  if (i == s.size())
    return false;

  for (; i < s.size(); ++i) {
    if (!is_digit(s[i]))
      return false;
  }

  return true;
}

const bool Tokenizer::is_nint(const std::string &s) const {
  if (s.size() < 2 || s[0] != '-') 
    return false;

  for (size_t i = 1; i < s.size(); ++i) {
    if (!is_digit(s[i]))
      return false;
  }

  return true;
}

const bool Tokenizer::is_float(const std::string &s) const {
  // Preprocess string for float compliance
  for (const auto c : s) {
    if (!is_digit(c) && c != '+' && c != '-' && c != '.') {
      return false;
    }
  }

  if (s.find_first_of(".eEnN") == std::string_view::npos)
    return false;

  return true;
}

const bool Tokenizer::is_date(const std::string &s) const {
  if (s.size() != 10)
    return false;
  if (!is_digit(s.at(0)))
    return false;
  if (!is_digit(s.at(1)))
    return false;
  if (!is_digit(s.at(2)))
    return false;
  if (!is_digit(s.at(3)))
    return false;
  if (s.at(4) != '-')
    return false;
  if (!is_digit(s.at(5)))
    return false;
  if (!is_digit(s.at(6)))
    return false;
  if (s.at(7) != '-')
    return false;
  if (!is_digit(s.at(8)))
    return false;
  if (!is_digit(s.at(9)))
    return false;
  // Must be YYYY-MM-DD format
  return true;
}

const bool Tokenizer::is_time(const std::string &s) const {
  // Determine if time present
  if (s.size() != 8)
    return false;
  if (!is_digit(s.at(0)))
    return false;
  if (!is_digit(s.at(1)))
    return false;
  if (s.at(2) != ':')
    return false;
  if (!is_digit(s.at(3)))
    return false;
  if (!is_digit(s.at(4)))
    return false;
  if (s.at(5) != ':')
    return false;
  if (!is_digit(s.at(6)))
    return false;
  if (!is_digit(s.at(7)))
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

  return true;
}

const bool Tokenizer::is_bool(const std::string &s) const {
  return (s == "true" || s == "false");
}

void Tokenizer::tokenize_file(const uint64_t segment_start, const uint64_t segment_size) {
  if (segment_size > std::numeric_limits<uint32_t>::max())
    throw std::out_of_range("Segment too large for 32-bit entry offsets");

  switch (type) {
    case FileType::CSV:
      tokenize_csv(segment_start, segment_size);
      break;
    case FileType::JSON:
      tokenize_json(segment_start, segment_size);
      break;
    case FileType::LOGFMT:
      tokenize_logfmt(segment_start, segment_size);
      break;
    default:
      throw std::invalid_argument("File type not supported");
  }
}

void Tokenizer::tokenize_csv(const uint64_t segment_start, const uint64_t segment_size) {
  // Open at front of file to get column names
  std::vector<std::string> col_names;
  {
    csv::CSVFormat format;
    format.variable_columns(csv::VariableColumnPolicy::THROW);
    col_names = csv::get_col_names(file_path, format);
  }
  
  // Reopen starting from segment_start
  auto in = std::make_unique<std::ifstream>(file_path, std::ios::binary);
  if (!*in) throw std::runtime_error("cannot open " + file_path);
  in->seekg(static_cast<std::streamoff>(segment_start));
  if (!*in) throw std::runtime_error("seek failed");

  csv::CSVFormat format;
  format.column_names(col_names)
        .delimiter(',')
        .variable_columns(csv::VariableColumnPolicy::THROW);

  // Header already read, so disregard it if reading from start of file
  if (segment_start == 0)
    format.header_row(0);

  csv::CSVReader reader(std::move(in), format); // 

  if (fields.empty())
    fields = col_names;

  // Resolve column names ids for faster lookup
  std::vector<size_t> field_idx;
  field_idx.reserve(fields.size());
  for (const auto &field : fields) {
    int idx = reader.index_of(field);
    if (idx < 0) throw std::runtime_error("unknown column: " + field);
    field_idx.push_back(static_cast<size_t>(idx));
  }

  const uint32_t num_fields = static_cast<uint32_t>(fields.size());
  for (const csv::CSVRow &row : reader) { // Input iterator
    uint64_t curr_offset = row.byte_offset();
    if (curr_offset >= segment_size) break; 

    for (uint32_t field_id = 0; field_id < num_fields; ++field_id )
      tokenize_field(row[field_idx[field_id]].get<std::string_view>(), entry_offsets.size(), field_id);

    entry_offsets.push_back(curr_offset);
  }

  entry_offsets.push_back(segment_size);
}

void Tokenizer::tokenize_json(const uint64_t segment_start, const uint64_t segment_size) {
  using namespace simdjson;
  constexpr size_t BATCH_SIZE = 16 << 20;

  padded_memory_map map(file_path.c_str());
  if (!map.is_valid()) throw std::runtime_error("cannot map " + file_path);
  padded_string_view json = map.view();

  ondemand::parser parser;
  ondemand::document_stream stream;
  auto error = parser.iterate_many(json.data() + segment_start, segment_size, BATCH_SIZE).get(stream);
  if (error) throw std::runtime_error(std::string("iterate_many: ") + error_message(error));

  for (auto j = stream.begin(); j != stream.end(); ++j) {
    auto doc = *j;
    ondemand::object obj;

    error = doc.get_object().get(obj);
    if (error) throw std::invalid_argument(std::string("JSON deserialisation error: ") + error_message(error));

    // If no fields add all fields
    // This assumes fields for every json object will be the same
    if (!fields.size()) {
      for (auto field : obj) {
        std::string_view f = field.escaped_key();
        fields.push_back(std::string(f));
      }
      
      if (obj.reset().error()) throw std::runtime_error("JSON reset failed");
    }

    for (auto field : obj) {
      std::string_view key = field.escaped_key();

      uint32_t field_id = 0;
      while (field_id < fields.size() && fields[field_id] != key) ++field_id;
      
      if (field_id == fields.size()) continue;

      ondemand::value v = field.value();
      ondemand::json_type type = v.type();

      std::string_view text;
      if (type == ondemand::json_type::string) {
        text = v.get_string();
      } else {
        text = v.raw_json();
      }

      tokenize_field(text, entry_offsets.size(), field_id);
    }

    entry_offsets.push_back(j.current_index());
  }

  entry_offsets.push_back(segment_size);
}

void Tokenizer::tokenize_logfmt(const uint64_t segment_start, const uint64_t segment_size) {
  LogfmtParser parser = LogfmtParser(file_path, segment_start, segment_start + segment_size);

  for (auto it = parser.begin(); it != parser.end(); ++it) {
    std::unordered_map<std::string, std::string> doc = (*it).current_row();

    // If no fields add all fields
    // This assumes fields for every logfmt object will be the same
    if (!fields.size()) {
      for (const auto &[key, _] : doc) { // Key : Value pair
        fields.push_back(key);
      }
    }

    for (uint32_t field_id = 0; field_id < fields.size(); ++field_id) {
      if (doc.contains(fields[field_id])) {
        tokenize_field(doc[fields[field_id]], entry_offsets.size(), field_id);
      } 
    }

    const int64_t raw = (*it).current_offset();
    if (raw < 0 || static_cast<uint64_t>(raw) < segment_start)
      throw std::invalid_argument("LOGFMT Parser: Invalid offset");

    const uint64_t rel = static_cast<uint64_t>(raw) - segment_start;
    if (rel > std::numeric_limits<uint32_t>::max())
      throw std::out_of_range("LOGFMT Parser: offset exceeds uint32_t range");

    entry_offsets.push_back(static_cast<uint32_t>(rel));
  }

  entry_offsets.push_back(segment_size);
}

void Tokenizer::tokenize_field(const std::string_view field_str,
                               const uint32_t doc_id, const uint32_t field_id) {
  field_token.clear();

  for (const char c : field_str) {
    if (std::isalnum(c) || c == '+' || c == '-' || c == '.' || c == ':') {
      field_token.push_back(c);
    } else if (field_token.size()) {
      processToken(field_token, doc_id, field_id);
      field_token.clear();
    }
  }

  // Process leftover token if present
  if (field_token.size())
    processToken(field_token, doc_id, field_id);
}

void Tokenizer::tokenize_string(const std::string &input) {
  field_token.clear();

  for (const char c : input) {
    if (std::isalnum(c) || c == '+' || c == '-' || c == '.' || c == ':') {
      field_token.push_back(c);
    } else if (field_token.size()) {
      processToken(field_token);
      field_token.clear();
    }
  }

  // Process leftover token if required
  if (field_token.size())
    processToken(field_token);
}

bool Tokenizer::handle_pint(std::string &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  if (!is_pint(curr_token)) 
    return false;

  std::uint64_t res_int;
  fast_float::from_chars_result res_err;
  fast_float::parse_options options{fast_float::chars_format::allow_leading_plus};

  res_err = fast_float::from_chars_advanced(curr_token.data(), curr_token.data() + curr_token.size(), res_int, options);
  if (res_err.ec == std::errc()) {
    if (res_err.ptr != curr_token.data() + curr_token.size())
      return false; // Didnt read the whole string

    if (res_int > UINT64_MAX - 1)
      return false;
    // File tokenization
    if (!query_mode) {
        vector_inserter(num_tokens[res_int][field_id], doc_id);
    // String tokenization
    } else {
        num_tokens_q.insert(res_int);
    }
  } else if (res_err.ec == std::errc::invalid_argument) {
    throw std::invalid_argument("Malformed number: \"" + curr_token);
  } else if (res_err.ec == std::errc::result_out_of_range) {
    return false;
  }

  return true;
}

bool Tokenizer::handle_nint(std::string &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  if (!is_nint(curr_token)) 
    return false;

  std::uint64_t res_int;
  fast_float::from_chars_result res_err;
  fast_float::parse_options options{fast_float::chars_format::allow_leading_plus};

  res_err = fast_float::from_chars_advanced(curr_token.data() + 1, curr_token.data() + curr_token.size(), res_int, options);
  if (res_err.ec == std::errc()) {
    if (res_err.ptr != curr_token.data() + curr_token.size())
      return false; // Didnt read the whole string
    
    if (res_int > UINT64_MAX - 1)
      return false;
      
    // File tokenization
    if (!query_mode)
        vector_inserter(neg_num_tokens[res_int][field_id], doc_id);
    // String tokenization
    else
        neg_num_tokens_q.insert(res_int);
  } else if (res_err.ec == std::errc::invalid_argument) {
    throw std::invalid_argument("Malformed number: \"" + curr_token);
  } else if (res_err.ec == std::errc::result_out_of_range) {
    return false;
  }

  return true;
}

bool Tokenizer::handle_float(std::string &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  if (!is_float(curr_token)) 
    return false;

  float res_float;
  fast_float::from_chars_result res_err;
  fast_float::parse_options options{fast_float::chars_format::allow_leading_plus};
  res_err = fast_float::from_chars_advanced(curr_token.data(), curr_token.data() + curr_token.size(), res_float, options);
  if (res_err.ec == std::errc()) {
    if (res_err.ptr != curr_token.data() + curr_token.size())
      return false; // Didnt read the whole string
    // File tokenization
    if (!query_mode)
      vector_inserter(float_tokens[std::move(res_float)][field_id], doc_id);
    // String tokenization
    else
      float_tokens_q.insert(res_float);
  } else {
    return false;
  }

  return true;
}

bool Tokenizer::handle_date(std::string &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  if (!is_date(curr_token)) 
    return false;
  
  std::istringstream in(curr_token);
  date::sys_time<std::chrono::seconds> dateTimePoint;

  // Check "YY-MM-DD format"
  date::from_stream(in, "%Y-%m-%d", dateTimePoint);
  // Check if parsable
  if (in.fail() || in.peek() != EOF) {
    return false;
  } else {
    // File tokenization
    if (!query_mode)
      vector_inserter(d_tokens[static_cast<uint64_t>(dateTimePoint.time_since_epoch().count())][field_id], doc_id);
    // String tokenization
    else
      d_tokens_q.insert(static_cast<uint64_t>(dateTimePoint.time_since_epoch().count()));
  }

  return true;
}

bool Tokenizer::handle_time(std::string &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  if (!is_time(curr_token)) 
    return false;

  std::istringstream in("1970-01-01 " + curr_token);
  std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> tp;
  date::from_stream(in, "%F %T", tp);

  // Check if parsable
  if (in.fail() || in.peek() != EOF) {
    return false;
  } else {
    // File tokenization
    if (!query_mode)
      vector_inserter(t_tokens[static_cast<uint64_t>(tp.time_since_epoch().count())][field_id], doc_id);
    // String tokenization
    else
      t_tokens_q.insert(static_cast<uint64_t>(tp.time_since_epoch().count()));
  }

  return true;
}

bool Tokenizer::handle_ipv4(std::string &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  if (!is_ipv4(curr_token)) 
    return false;

  // File tokenization
  bool fail;
  uint32_t ip_addr = ip_str_to_int(2 /* AF_INET */, curr_token, fail);

  if (fail) return false;
  if (!query_mode)
    vector_inserter(ipv4_tokens[ip_addr][field_id], doc_id);
  // String tokenization
  else
    ipv4_tokens_q.insert(ip_addr);

  return true;
}

bool Tokenizer::handle_bool(std::string  &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  if (!is_bool(curr_token)) 
    return false;

  // File tokenization
  if (!query_mode) {
    if (curr_token == "true") {
      vector_inserter(t_bool_tokens[field_id], doc_id);
    } else if (curr_token == "false") {
       vector_inserter(f_bool_tokens[field_id], doc_id);
    }
  // String tokenization
  } else {
    if (curr_token == "true")
      t_bool_token_q = true;
    if (curr_token == "false")
      f_bool_token_q = true;
  }

  return true;
};

void Tokenizer::handle_alpha(std::string  &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  // Fast path
  if (std::ranges::all_of(curr_token, is_alpha)) {
    if (curr_token.size() <= MAX_TOK_LENGTH) {
      std::ranges::transform(curr_token, curr_token.begin(), to_lower);
      if (!query_mode)
        vector_inserter(alpha_tokens[curr_token][field_id], doc_id);
      else
        alpha_tokens_q.insert(curr_token);
    }

    return;
  }

  std::string curr;
  bool split = false;
  for (const char c : curr_token) {
    if (curr.size() == 0 && is_alnum(c)) {
      curr.push_back(c);
    } else if (curr.size() && is_digit(c) && is_digit(curr.back())) {
      curr.push_back(c);
    } else if (curr.size() && is_alpha(c) && is_alpha(curr.back())) {
      curr.push_back(c);
    } else {
      split = true;
      if (!curr.empty()) {
        if (!query_mode)
          processToken(curr, doc_id, field_id);
        else
          processToken(curr);
        curr.clear();
      }

      if (is_alnum(c)) {
        curr.push_back(c);
      }
    }
  }

  if (curr.empty()) return;

  if (split) {
    if (!query_mode)
      processToken(curr, doc_id, field_id);
    else
      processToken(curr);
  } else {
    if (curr.size() <= MAX_TOK_LENGTH) {
      std::ranges::transform(curr.begin(), curr.end(), curr.begin(), [](char c) { return to_lower(c); });
      if (!query_mode)
        vector_inserter(alpha_tokens[std::move(curr)][field_id], doc_id);
      else
        alpha_tokens_q.insert(std::move(curr));
    }
  }
}

void Tokenizer::processToken(std::string &curr_token, const uint32_t doc_id, const uint32_t field_id) {
  if (handle_bool(curr_token, doc_id, field_id)) return;

  // Simple way to prevent other more expensive type checking in the below handle_x fuctions
  if (contains_alpha(curr_token)) {
    handle_alpha(curr_token, doc_id, field_id);
    return;
  }

  if (handle_pint(curr_token, doc_id, field_id)) return;
  if (handle_nint(curr_token, doc_id, field_id)) return;
  if (handle_float(curr_token, doc_id, field_id)) return;
  if (handle_date(curr_token, doc_id, field_id)) return;
  if (handle_time(curr_token, doc_id, field_id)) return;
  if (handle_ipv4(curr_token, doc_id, field_id)) return;

  // Second handle_alpha call needed as some of these above handle_x functions can fail due to type size limits
  handle_alpha(curr_token, doc_id, field_id);
}

void Tokenizer::processToken(std::string &curr_token) {
  if (handle_bool(curr_token)) return;
  
  // Simple way to prevent other more expensive type checking in the below handle_x fuctions
  if (contains_alpha(curr_token)) {
    handle_alpha(curr_token);
    return;
  }

  if (handle_pint(curr_token)) return;
  if (handle_nint(curr_token)) return;
  if (handle_float(curr_token)) return;
  if (handle_date(curr_token)) return;
  if (handle_time(curr_token)) return;
  if (handle_ipv4(curr_token)) return;

  // Second handle_alpha call needed as some of these above handle_x functions can fail due to type size limits
  handle_alpha(curr_token);
}

void Tokenizer::vector_inserter(std::vector<uint32_t> &vec, const uint32_t value) {
  if (vec.empty() || vec.back() != value) vec.push_back(value);
}

void Tokenizer::printAlphaTokens() const {
  for (const auto &[key, _] : alpha_tokens)
    std::cout << key << std::endl;
}

void Tokenizer::printNumTokens() const {
  for (const auto &[key, _] : num_tokens)
    std::cout << key << std::endl;
}

void Tokenizer::printNegNumTokens() const {
  for (const auto &[key, _] : neg_num_tokens)
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

Tokenizer Tokenizer::from_file(const std::string &file_path, const std::vector<std::string> &fields, const uint64_t segment_start, uint64_t segment_size) {
  Tokenizer t;

  if (segment_size == 0)
    segment_size = std::filesystem::file_size(std::filesystem::path(file_path));

  t.file_path = file_path;
  t.fields = fields;
  t.type = str_to_file_type(file_path);
  t.field_token.reserve(MAX_TOK_LENGTH);
  t.tokenize_file(segment_start, segment_size);

  return t;
}

Tokenizer Tokenizer::from_string(const std::string &file_path) {
  Tokenizer t;
  t.query_mode = true;
  t.field_token.reserve(MAX_TOK_LENGTH);
  t.tokenize_string(file_path);

  return t;
}

void Tokenizer::printStatistics() const {
  std::cout << "Total Alpha Tokens: " << alpha_tokens.size() << std::endl;
  std::cout << "Total Positive Integer Tokens: " << num_tokens.size() << std::endl;
  std::cout << "Total Negative Integer Tokens: " << neg_num_tokens.size() << std::endl;
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

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}

const std::vector<uint64_t> Tokenizer::getNumTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : num_tokens)
    tokens.push_back(i.first);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}

const std::vector<uint64_t> Tokenizer::getNegNumTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : neg_num_tokens)
    tokens.push_back(i.first);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}

const std::vector<float> Tokenizer::getFloatTokens() const {
  std::vector<float> tokens;
  for (const auto i : float_tokens)
    tokens.push_back(i.first);
  
  std::sort(tokens.begin(), tokens.end());

  return tokens;
}

const std::vector<uint64_t> Tokenizer::getDateTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : d_tokens)
    tokens.push_back(i.first);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getTimeTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : t_tokens)
    tokens.push_back(i.first);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getIPTokens() const {
  std::vector<uint64_t> tokens;
  for (const auto i : ipv4_tokens)
    tokens.push_back(i.first);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}

const std::vector<std::string> Tokenizer::getAlphaTokensQ() const {
  std::vector<std::string> tokens;
  for (const auto &i : alpha_tokens_q)
    tokens.push_back(i);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}

const std::vector<uint64_t> Tokenizer::getNumTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : num_tokens_q)
    tokens.push_back(i);
  
  std::sort(tokens.begin(), tokens.end());

  return tokens;
}

const std::vector<uint64_t> Tokenizer::getNegNumTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : neg_num_tokens_q)
    tokens.push_back(i);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}

const std::vector<float> Tokenizer::getFloatTokensQ() const {
  std::vector<float> tokens;
  for (const auto i : float_tokens_q)
    tokens.push_back(i);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getDateTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : d_tokens_q)
    tokens.push_back(i);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getTimeTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : t_tokens_q)
    tokens.push_back(i);

  std::sort(tokens.begin(), tokens.end());

  return tokens;
}
const std::vector<uint64_t> Tokenizer::getIPTokensQ() const {
  std::vector<uint64_t> tokens;
  for (const auto i : ipv4_tokens_q)
    tokens.push_back(i);

  std::sort(tokens.begin(), tokens.end());
  
  return tokens;
}