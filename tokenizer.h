#include <cstdio>
#include <sys/stat.h>
#include <time.h>
#include "fast_float/fast_float.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include "date/date.h"
#include <chrono>

#ifndef INDEX_H
#define INDEX_H

namespace NumericType {
    enum Value : uint8_t {
        NONE = 0,
        INT = 1,
        FLOAT = 2,
        DATE = 3,
        TIME = 4,
        IPV4 = 5,
        IPV6 = 6,
    };
}

class Tokenizer {
    public:
        Tokenizer(const std::string &file_path);
        void close();
        std::unordered_map<std::string, std::vector<uint64_t>> alpha_tokens;
        std::unordered_map<uint64_t, std::vector<uint64_t>> num_tokens;
        std::unordered_map<float, std::vector<uint64_t>> float_tokens;
        std::unordered_map<time_t, std::vector<uint64_t>> d_tokens;
        std::unordered_map<time_t, std::vector<uint64_t>> t_tokens;
        std::unordered_map<uint32_t, std::vector<uint64_t>> ipv4_tokens;
        std::unordered_map<uint64_t, std::vector<uint64_t>> ipv6_tokens;
        std::vector<uint64_t> offsets;

        void printAlphaTokens();
        void printNumTokens();
        void printFloatTokens();
        void printDateTokens();
        void printTimeTokens();
        void printIPTokens();

    private:
        void tokenize();
        void processToken(std::string &curr_token, uint64_t &offset);
        const long get_file_size(const std::string &filename);
        const uint8_t numeric_type(const std::string& s);
        const bool is_ipv6(const std::string& s);
        const bool is_ipv4(const std::string& s);
        const bool is_time(const std::string& s);
        const bool is_date(const std::string& s);
        const bool is_float(const std::string& s);
        const bool is_int(const std::string& s);

        FILE *input_file;
        const size_t BUFFER_SIZE = 16384;
};
#endif