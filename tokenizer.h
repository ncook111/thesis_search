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
        std::vector<uint8_t*> alpha_tokens;
        std::vector<uint32_t> num_tokens;
        std::vector<float> float_tokens;
        std::vector<time_t> d_tokens;
        std::vector<time_t> t_tokens;
        std::vector<uint32_t> ipv4_tokens;
        std::vector<uint64_t> ipv6_tokens;

    private:
        void tokenize();
        void processToken(std::string &curr_token);
        const long get_file_size(const std::string &filename);
        const uint8_t numeric_type(const std::string& s);
        const bool is_ipv6(const std::string& s);
        const bool is_ipv4(const std::string& s);
        const bool is_time(const std::string& s);
        const bool is_date(const std::string& s);
        const bool is_float(const std::string& s);
        const bool is_int(const std::string& s);

        FILE *input_file;
        uint8_t type;
};
#endif