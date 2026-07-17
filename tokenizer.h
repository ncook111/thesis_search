#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <cstdio>
#include <sys/stat.h>
#include <time.h>
#include "fast_float/fast_float.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include "date/date.h"
#include <chrono>
#include <map>

namespace TokenType {
    enum Value : uint8_t {
        ALPHA = 0,
        INT = 1,
        FLOAT = 2,
        DATE = 3,
        TIME = 4,
        IPV4 = 5,
        IPV6 = 6,
        BOOLEAN = 7,
        INVALID = 8,
    };
}

class Tokenizer {
    public:
        static Tokenizer from_file(const std::string &file_path, const uint8_t type); // Takes in path to file in which to tokenize
        static Tokenizer from_string(const std::string &input); // Takes in string in which to tokenize
        void close();

        std::map<std::string, std::vector<uint64_t>> alpha_tokens;
        std::map<uint64_t, std::vector<uint64_t>> num_tokens;
        std::map<uint64_t, std::vector<uint64_t>> neg_num_tokens;
        std::map<float, std::vector<uint64_t>> float_tokens;
        std::map<uint64_t, std::vector<uint64_t>> d_tokens;
        std::map<uint64_t, std::vector<uint64_t>> t_tokens;
        std::map<uint64_t, std::vector<uint64_t>> ipv4_tokens;
        std::map<uint64_t, std::vector<uint64_t>> ipv6_tokens;
        std::vector<bool> boolean_tokens;
        std::vector<uint64_t> entry_offsets; // Denotes the start point for all entries in the file (newline for csv and logfmt, {} for json)

        void printAlphaTokens() const;
        void printNumTokens() const;
        void printFloatTokens() const;
        void printDateTokens() const;
        void printTimeTokens() const;
        void printIPTokens() const;
        void printStatistics() const;

        const std::vector<std::string> getAlphaTokens() const;
        const std::vector<uint64_t> getNumTokens() const;
        const std::vector<float> getFloatTokens() const;
        const std::vector<uint64_t> getDateTokens() const;
        const std::vector<uint64_t> getTimeTokens() const;
        const std::vector<uint64_t> getIPTokens() const;

    private:
        Tokenizer() = default;
        void tokenize_file();
        void tokenize_string(const std::string &input);
        void processToken(std::string &curr_token, const uint64_t offset);
        void tokenize_csv();
        void tokenize_json();
        void tokenize_logfmt();
        void generate_entry_offsets();
        void generate_entry_offsets_csv();
        void generate_entry_offsets_json();
        inline const uint8_t numeric_type(const std::string& s) const;
        const bool is_ipv6(const std::string& s) const;
        const bool is_ipv4(const std::string& s) const;
        const bool is_time(const std::string& s) const;
        const bool is_date(const std::string& s) const;
        const bool is_float(const std::string& s) const;
        const bool is_int(const std::string& s) const;
        const bool is_bool(const std::string& s) const;

        std::string file_path;
        uint8_t type;
};

#endif