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
        std::map<int64_t, std::vector<uint64_t>> num_tokens;
        std::map<float, std::vector<uint64_t>> float_tokens;
        std::map<time_t, std::vector<uint64_t>> d_tokens;
        std::map<time_t, std::vector<uint64_t>> t_tokens;
        std::map<uint32_t, std::vector<uint64_t>> ipv4_tokens;
        std::map<uint64_t, std::vector<uint64_t>> ipv6_tokens;
        std::vector<bool> boolean_tokens;
        std::vector<std::pair<uint64_t, uint64_t>> entry_offsets; // Denotes the start and end points of all entries within the file (newline for csv and logfmt, {} for json)

        void printAlphaTokens();
        void printNumTokens();
        void printFloatTokens();
        void printDateTokens();
        void printTimeTokens();
        void printIPTokens();
        void printStatistics();

        std::vector<std::string> getAlphaTokens();
        std::vector<int64_t> getNumTokens();
        std::vector<float> getFloatTokens();
        std::vector<time_t> getDateTokens();
        std::vector<time_t> getTimeTokens();
        std::vector<uint32_t> getIPTokens();

    private:
        Tokenizer() = default;
        void tokenize_file(void *file_addr, const size_t &file_size, const uint8_t type);
        void tokenize_string(const std::string &input);
        void processToken(std::string &curr_token, const uint64_t offset);
        inline const uint8_t numeric_type(const std::string& s);
        const bool is_ipv6(const std::string& s);
        const bool is_ipv4(const std::string& s);
        const bool is_time(const std::string& s);
        const bool is_date(const std::string& s);
        const bool is_float(const std::string& s);
        const bool is_int(const std::string& s);
        const bool is_bool(const std::string& s);

        uint8_t type;
};

#endif