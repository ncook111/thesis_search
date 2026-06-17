#include <iostream>
#include <cstdint>
#include <cstddef>

int8_t vb_encode_num(std::array<uint8_t, 5> &bytes, uint32_t num);
std::vector<uint8_t> vb_encode(const std::vector<uint32_t> &nums);
std::vector<uint32_t> vb_decode(const std::vector<uint8_t> &bytes);
std::vector<uint8_t> gvb_encode_uint32_t(const std::vector<uint32_t> &nums);
std::vector<uint32_t> gvb_decode_uint32_t(const std::vector<uint8_t> &bytes);
std::vector<uint8_t> gvb_encode_uint64_t(const std::vector<uint64_t> &nums);
std::vector<uint64_t> gvb_decode_uint64_t(const std::vector<uint8_t> &bytes);
inline const uint8_t get_length(const uint64_t num);
inline const uint8_t get_length(const uint32_t num);
void check(std::vector<uint32_t> &nums_in);
long getMemoryUsage();