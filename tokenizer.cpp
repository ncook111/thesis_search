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

const bool Tokenizer::is_int(const std::string& s) {
    if (s.size() == 1 && std::isdigit(s.front())) {
        return true;
    } else if (s.size() == 1) {
        return false;
    }

    if (std::find_if(s.begin()+1, s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end()) {
        if (s.front() == '+' || std::isdigit(s.front())) {
            return true;
        }

        if (s.front() == '-') {
            return true;
        }
    }

    return false;
}

const bool Tokenizer::is_float(const std::string& s) {
    // Preprocess string for float compliance
    for (auto it = s.begin(); it != s.end(); ++it) {
        const char c = *it;
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

const bool Tokenizer::is_date(const std::string& s) {
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

const bool Tokenizer::is_time(const std::string& s) {
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

const bool Tokenizer::is_ipv4(const std::string& s) {
    if (s.size() < 7 || s.size() > 15) return false; // Max length of ipv4

    // Check only valid chars
    for (auto it = s.begin(); it != s.end(); ++it) {
        const char c = *it;
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

    for (auto it = parts.begin(); it != parts.end(); ++it) {
        const std::string curr = (*it);
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

const bool Tokenizer::is_ipv6(const std::string& s) {
    return false;
}

// Doesn't handle + or - symbols yet
const uint8_t Tokenizer::numeric_type(const std::string& s) {
    if (is_date(s)) { return NumericType::DATE; }
    if (is_time(s)) { return NumericType::TIME; }
    if (is_int(s)) { return NumericType::INT; }
    if (is_float(s)) { return NumericType::FLOAT; }
    if (is_ipv4(s)) { return NumericType::IPV4;}

    return NumericType::NONE;
}

const long Tokenizer::get_file_size(const std::string &filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

void Tokenizer::tokenize() {
    uint8_t *buffer = (uint8_t*)malloc(BUFFER_SIZE);
    std::string curr_token;
    uint64_t total_bytes_read = 0;
    uint64_t curr_tok_offset = 0;

    while (size_t bytes_read = fread(buffer, 1, BUFFER_SIZE, input_file)) {
        for (size_t i = 0; i < bytes_read; ++i) {  
            if (isalnum(buffer[i])
                || buffer[i] == '+'
                || buffer[i] == '-'
                || buffer[i] == '.'
                || buffer[i] == ':') {
                curr_token.push_back(buffer[i]);
            } else if (curr_token.size() != 0) {
                processToken(curr_token, curr_tok_offset);
                offsets.push_back(curr_tok_offset);
                curr_tok_offset = total_bytes_read + i;
            }
        }

        total_bytes_read += bytes_read;
    }

    // Process leftover token if required
    if (curr_token.size()) {
        processToken(curr_token, curr_tok_offset);
        offsets.push_back(curr_tok_offset);
    }

    free(buffer);
    std::cout << "Total bytes read: " << total_bytes_read << std::endl;
}

void Tokenizer::processToken(std::string &curr_token, uint64_t &offset) {
    const uint32_t token_length = curr_token.size();
    if (token_length) {
        bool unprocessed = false;
        std::istringstream in(curr_token);
        fast_float::parse_options options{fast_float::chars_format::allow_leading_plus};
        fast_float::from_chars_result res_err;
        date::sys_time<std::chrono::seconds> dateTimePoint;
        std::chrono::seconds timeTimePoint;

        // Deal with an alphabetical string and numeric string differently
        switch (numeric_type(curr_token)) {
            case NumericType::INT:
                std::int64_t res_int;
                res_err = fast_float::from_chars_advanced(curr_token.data(), curr_token.data() + token_length, res_int, options);
                if (res_err.ec == std::errc()) {
                    num_tokens[res_int].push_back(offset);
                } else if (res_err.ec == std::errc::invalid_argument) {
                    std::cout << "Malformed number: \"" << curr_token << "\"" << std::endl;
                } else if (res_err.ec == std::errc::result_out_of_range) {
                    unprocessed = true;
                }

                break;
                
            case NumericType::FLOAT:
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

            case NumericType::DATE:
                // Check "YY-MM-DD format"
                in >> date::parse("%Y-%m-%d", dateTimePoint);
                // Check if parsable
                if (in.fail() || in.peek() != EOF) {
                    unprocessed = true;
                } else {
                    d_tokens[dateTimePoint.time_since_epoch().count()].push_back(offset);
                }
                break;
            case NumericType::TIME:
                // Check "HH-MM-SS format"
                in >> date::parse("%T", timeTimePoint);
                // Check if parsable
                if (in.fail() || in.peek() != EOF) {
                    unprocessed = true;
                } else {
                    t_tokens[timeTimePoint.count()].push_back(offset);
                }
                break;

            case NumericType::IPV4:
                ipv4_tokens[1].push_back(offset);
                break;

            case NumericType::IPV6:
                ipv6_tokens[1].push_back(offset);
                break;

            default: 
                unprocessed = true;
                break;
        }

        if (unprocessed) {
            std::transform(curr_token.begin(), curr_token.end(), curr_token.begin(),[](unsigned char c){ return std::tolower(c); });
            alpha_tokens[std::move(curr_token)].push_back(offset);
        }

        curr_token.erase();
    }
}

void Tokenizer::printAlphaTokens() {
    for (const auto& [key, _] : alpha_tokens) {
        std::cout << key << std::endl;;
    }
}

void Tokenizer::printNumTokens() {
    for (const auto& [key, _] : num_tokens) {
        std::cout << key << std::endl;
    }
}

void Tokenizer::printFloatTokens() {
    for (const auto& [key, _] : float_tokens) {
        std::cout << key << std::endl;
    }
}

void Tokenizer::printDateTokens() {
    for (const auto& [key, _] : d_tokens) {
        std::tm* tm_info = std::gmtime(&(key));
        std::ostringstream oss;
        oss << std::put_time(tm_info, "%Y-%m-%d");
        std::cout << oss.str() << std::endl;
    }
}

void Tokenizer::printTimeTokens() {
    for (const auto& [key, _] : t_tokens) {
        std::tm* tm_info = std::gmtime(&(key));
        std::ostringstream oss;
        oss << std::put_time(tm_info, "%T");
        std::cout << oss.str() << std::endl;; 
    }
}

void Tokenizer::printIPTokens() {

}

Tokenizer::Tokenizer(const std::string &file_path) {
    input_file = fopen(file_path.c_str(), "r");

    if (!input_file) {
        std::cerr << "Input file opening failed\n";
        exit(EXIT_FAILURE);
    }

    tokenize();

    fclose(input_file);
}

void Tokenizer::close() {
    // for (auto it = alpha_tokens.begin(); it != alpha_tokens.end(); ++it) {
    //     free(*it);
    // }
}