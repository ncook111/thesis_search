#include <cstdio>
#include <sys/stat.h>
#include <time.h>
#include "fast_float/fast_float.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include "date/date.h"
#include <chrono>
#include "tokenizer.h"
#include "ingest.h"
#include "coder.h"
#include "helper.h"
#include <fstream>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "csv.hpp"
#include <string_view>

const bool Tokenizer::is_int(const std::string &s) const {
    size_t i = s.front() == '+' || s.front() == '-' ? 1 : 0;

    if (i == s.size()) return false;

    for (; i < s.size(); ++i) {
        if (!std::isdigit(s[i])) return false;
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
        size_t processed; // Number of characters processed, must match string size to be valid float
        std::stof(s, &processed);
        
        if (processed == s.size()) return true;

    } catch (const std::invalid_argument) {
        return false;
    } catch (const std::out_of_range) {
        return false;
    }

    return false;
}

const bool Tokenizer::is_date(const std::string &s) const {
    if (s.size() != 10) return false;
    if (!std::isdigit(s.at(0))) return false;
    if (!std::isdigit(s.at(1))) return false;
    if (!std::isdigit(s.at(2))) return false;
    if (!std::isdigit(s.at(3))) return false;
    if (s.at(4) != '-')         return false;
    if (!std::isdigit(s.at(5))) return false;
    if (!std::isdigit(s.at(6))) return false;
    if (s.at(7) != '-')         return false;
    if (!std::isdigit(s.at(8))) return false;
    if (!std::isdigit(s.at(9))) return false;
    // Must be YYYY-MM-DD format
    return true;
}

const bool Tokenizer::is_time(const std::string &s) const {
    // Determine if time present
    if (s.size() != 8) return false;
    if (!std::isdigit(s.at(0))) return false;
    if (!std::isdigit(s.at(1))) return false;
    if (s.at(2) != ':') return false;
    if (!std::isdigit(s.at(3))) return false;
    if (!std::isdigit(s.at(4))) return false;
    if (s.at(5) != ':') return false;
    if (!std::isdigit(s.at(6))) return false;
    if (!std::isdigit(s.at(7))) return false;

    return true;
}

const bool Tokenizer::is_ipv4(const std::string &s) const {
    if (s.size() < 7 || s.size() > 15) return false; // Max length of ipv4

    // Check only valid chars
    for (const auto c : s) {
        if (std::isdigit(c) || c == '.') continue;
        else return false;
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

    if (parts.size() != 4) return false;

    for (const auto curr : parts) {
        std::uint64_t res_int;
        fast_float::from_chars_result res_err = fast_float::from_chars(curr.data(), curr.data() + curr.size(), res_int);
        if (res_err.ec == std::errc()) {
            if (res_int > 256) return false;
        } else if (res_err.ec == std::errc::invalid_argument) {
            return false;
        } else if (res_err.ec == std::errc::result_out_of_range) {
            return false;
        }
    }

    return true;
}

const bool Tokenizer::is_ipv6(const std::string &s) const {
    return false;
}

const bool Tokenizer::is_bool(const std::string& s) const {
    return (s == "true" || s == "false");
}

inline const uint8_t Tokenizer::numeric_type(const std::string &s) const {
    if (is_int(s)) { return TokenType::INT; }
    if (is_float(s)) { return TokenType::FLOAT; }
    if (is_date(s)) { return TokenType::DATE; }
    if (is_time(s)) { return TokenType::TIME; }
    if (is_ipv4(s)) { return TokenType::IPV4; }
    if (is_bool(s)) { return TokenType::BOOLEAN; }
    return TokenType::ALPHA;
}

void Tokenizer::tokenize_file(const std::string &file_path, const uint8_t type) {
    csv::CSVFormat format;
    format.variable_columns(csv::VariableColumnPolicy::THROW);
    csv::CSVReader reader(file_path, format);

    // Set up entry_offsets with inital value of 1
    // Offsets required to start at 1
    entry_offsets.push_back(1);

    // Line scanner preprocessing
    // Only checking for newlines atm
    const MappedFile mf = open_mmap_r(file_path);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();

    for (size_t i = 1; i < file_size; ++i) {
        if (file[i] == '\n')
            entry_offsets.push_back(i + 2); // Start offset 1 after newline
        else if (file[i-1] == '\r')
            entry_offsets.push_back(i + 1); // Start offset 1 after carriage return
    }

    entry_offsets.push_back(file_size + 2);

    std::string curr_token;
    uint64_t i = 1; // Start from 1 to account for header row
    for (const csv::CSVRow &row: reader) { // Input iterator

        for (csv::CSVField &field: row) {
            std::string_view line = field.get<std::string_view>();
            for (const auto c : line) {
                if (isalnum(c)
                    || c == '+'
                    || c == '-'
                    || c == '.'
                    || c == ':') {
                    curr_token.push_back(c);
                } else {
                    if (curr_token.size()) {
                        processToken(curr_token, i);
                    }
                }
            }

            // Process leftover token if present
            if (curr_token.size()) {
                processToken(curr_token, i); // Offsets begin at 1 as a requirement of the gvb encoding and decoding steps
            }
        }
        if (i % 10000 == 0)
            std::cout << "Processed: " << (float)(i/(float)entry_offsets.size()) * 100 << "%" << std::endl;
       
        ++i;
    }

    // Process leftover token if present
    if (curr_token.size()) {
        processToken(curr_token, entry_offsets.size()); // Offsets begin at 1 as a requirement of the gvb encoding and decoding steps
    }

    std::cout << "Lines: " << entry_offsets.size() << std::endl;
    std::cout << "Total bytes read: " << file_size << std::endl;
}

void Tokenizer::tokenize_string(const std::string &input) {
    std::string curr_token;
    const size_t input_size = input.size();

    for (size_t i = 0; i < input_size; ++i) {  
        if (isalnum(input[i])
            || input[i] == '+'
            || input[i] == '-'
            || input[i] == '.'
            || input[i] == ':') {
            curr_token.push_back(input[i]);
        } else if (curr_token.size()) {
            processToken(curr_token, i - curr_token.size() + 1); // Offsets begin at 1 as a requirement of the gvb encoding and decoding steps
        }
    }

    // Process leftover token if required
    if (curr_token.size()) {
        processToken(curr_token, input_size - curr_token.size() + 1); // Offsets begin at 1 as a requirement of the gvb encoding and decoding steps
    }
}

void Tokenizer::processToken(std::string &curr_token, const uint64_t offset) {
    const uint32_t token_length = curr_token.size();
    if (token_length) {
        bool unprocessed = false;
        std::istringstream in(curr_token);
        fast_float::parse_options options{fast_float::chars_format::allow_leading_plus};
        fast_float::from_chars_result res_err;
        date::sys_time<std::chrono::seconds> dateTimePoint;
        std::chrono::seconds timeTimePoint;

        std::transform(curr_token.begin(), curr_token.end(), curr_token.begin(),[](unsigned char c){ return std::tolower(c); });

        // Deal with an alphabetical string and numeric string differently
        switch (numeric_type(curr_token)) {
            case TokenType::INT:
                std::int64_t res_int;
                res_err = fast_float::from_chars_advanced(curr_token.data(), curr_token.data() + token_length, res_int, options);
                if (res_err.ec == std::errc()) {
                    if (res_int >= 0)
                        num_tokens[res_int].push_back(offset);
                    else {
                        num_tokens[res_int * -1].push_back(offset);
                    }
                } else if (res_err.ec == std::errc::invalid_argument) {
                    std::cout << "Malformed number: \"" << curr_token << "\"" << std::endl;
                } else if (res_err.ec == std::errc::result_out_of_range) {
                    unprocessed = true;
                }

                break;
                
            case TokenType::FLOAT:
                float res_float;
                res_err = fast_float::from_chars_advanced(curr_token.data(), curr_token.data() + token_length, res_float, options);
                if (res_err.ec == std::errc()) {
                    float_tokens[std::move(res_float)].push_back(offset);
                } else if (res_err.ec == std::errc::invalid_argument) {
                    std::cout << "Malformed number: \"" << curr_token << "\"" << std::endl;
                } else if (res_err.ec == std::errc::result_out_of_range) {
                    unprocessed = true;
                }

                break;

            case TokenType::DATE:
                // Check "YY-MM-DD format"
                in >> date::parse("%Y-%m-%d", dateTimePoint);
                // Check if parsable
                if (in.fail() || in.peek() != EOF) {
                    unprocessed = true;
                } else {
                    d_tokens[static_cast<uint64_t>(dateTimePoint.time_since_epoch().count())].push_back(offset);
                }
                break;
            case TokenType::TIME:
                // Check "HH-MM-SS format"
                in >> date::parse("%T", timeTimePoint);
                // Check if parsable
                if (in.fail() || in.peek() != EOF) {
                    unprocessed = true;
                } else {
                    t_tokens[static_cast<uint64_t>(timeTimePoint.count())].push_back(offset);
                }
                break;

            case TokenType::IPV4:
                ipv4_tokens[1].push_back(offset);
                break;

            case TokenType::IPV6:
                ipv6_tokens[1].push_back(offset);
                break;

            case TokenType::BOOLEAN:
                if (curr_token == "true") boolean_tokens.push_back(true);
                if (curr_token == "false") boolean_tokens.push_back(false);
                break;

            default: 
                unprocessed = true;
                break;
        }

        if (unprocessed) {
            alpha_tokens[std::move(curr_token)].push_back(offset);
        }

        curr_token.erase();
    }
}

void Tokenizer::printAlphaTokens() const {
    for (const auto &[key, _] : alpha_tokens) {
        std::cout << key << std::endl;;
    }
}

void Tokenizer::printNumTokens() const {
    for (const auto &[key, _] : num_tokens) {
        std::cout << key << std::endl;
    }
}

void Tokenizer::printFloatTokens() const {
    for (const auto &[key, _] : float_tokens) {
        std::cout << key << std::endl;
    }
}

void Tokenizer::printDateTokens() const {
    for (const auto &[key, _] : d_tokens) {
        time_t tm = key;
        std::tm* tm_info = std::gmtime(&tm);
        std::ostringstream oss;
        oss << std::put_time(tm_info, "%Y-%m-%d");
        std::cout << oss.str() << std::endl;
    }
}

void Tokenizer::printTimeTokens() const {
    for (const auto &[key, _] : t_tokens) {
        time_t tm = key;
        std::tm* tm_info = std::gmtime(&tm);
        std::ostringstream oss;
        oss << std::put_time(tm_info, "%T");
        std::cout << oss.str() << std::endl;; 
    }
}

void Tokenizer::printIPTokens() const {

}

Tokenizer Tokenizer::from_file(const std::string &file_path, const uint8_t type) {
    Tokenizer t;
    t.tokenize_file(file_path, type);

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
    std::cout << "Total Boolean Tokens: " << boolean_tokens.size() << std::endl;
}

const std::vector<std::string> Tokenizer::getAlphaTokens() const {
    std::vector<std::string> tokens;
    for (const auto &i: alpha_tokens) {
        tokens.push_back(i.first);
    }

    return tokens;
}

const std::vector<uint64_t> Tokenizer::getNumTokens() const {
    std::vector<uint64_t> tokens;
    for (const auto i: num_tokens) {
        tokens.push_back(i.first);
    }

    return tokens;
}
const std::vector<float> Tokenizer::getFloatTokens() const {
    std::vector<float> tokens;
    for (const auto i: float_tokens) {
        tokens.push_back(i.first);
    }

    return tokens;
}
const std::vector<uint64_t> Tokenizer::getDateTokens() const {
    std::vector<uint64_t> tokens;
    for (const auto i: d_tokens) {
        tokens.push_back(i.first);
    }

    return tokens;
}
const std::vector<uint64_t> Tokenizer::getTimeTokens() const {
    std::vector<uint64_t> tokens;
    for (const auto i: t_tokens) {
        tokens.push_back(i.first);
    }

    return tokens;
}
const std::vector<uint64_t> Tokenizer::getIPTokens() const {
        std::vector<uint64_t> tokens;
    for (const auto i: ipv4_tokens) {
        tokens.push_back(i.first);
    }

    return tokens;
}