#include "ingest.h"
#include "coder.h"
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

int main(int argc, char *argv[]) {
    const std::string file_path = argv[1];

    Tokenizer tokenizer(file_path);

    // std::vector<uint8_t> encoded = gvb_encode_uint32_t(tokenizer.num_tokens);
    // std::vector<uint32_t> decoded = gvb_decode_uint32_t(encoded);

    // std::cout << "Size compressed: " << encoded.size() << " bytes" << std::endl;
    // std::cout << "Size uncompressed: " << tokenizer.num_tokens.size() * 4 << " bytes" << std::endl;
    // std::cout << "Compression Ratio: " << (float)encoded.size() / (float)(tokenizer.num_tokens.size() * 4) * 100 << "%" << std::endl;

    // if (tokenizer.num_tokens != decoded) {
    //     std::cout << "Incorrect" << std::endl;
    // } else {
    //     std::cout << "Correct" << std::endl;
    // }

    tokenizer.close();

    std::cout << "Memory Usage: " << getMemoryUsage() << std::endl;
    std::cout << "Total Alpha Tokens: " << tokenizer.alpha_tokens.size() << std::endl;
    std::cout << "Total Integer Tokens: " << tokenizer.num_tokens.size() << std::endl;
    std::cout << "Total Float Tokens: " << tokenizer.float_tokens.size() << std::endl;
    std::cout << "Total Date Tokens: " << tokenizer.d_tokens.size() << std::endl;
    std::cout << "Total Time Tokens: " << tokenizer.t_tokens.size() << std::endl;
    std::cout << "Total Ipv4 Tokens: " << tokenizer.ipv4_tokens.size() << std::endl;
    std::cout << "Total Ipv6 Tokens: " << tokenizer.ipv6_tokens.size() << std::endl;
    std::cout << "Total offsets: " << tokenizer.offsets.size() << std::endl;

    std::vector<uint64_t> nums = tokenizer.alpha_tokens["female"];
    std::cout << "Size: " << nums.size() << std::endl;
    // for (const auto& num : nums) {
    //     std::cout << num << std::endl;
    // }
}