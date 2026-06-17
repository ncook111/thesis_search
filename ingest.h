#include <iostream>
#include <cstdint>
#include <cstddef>

void vb_encode_num(std::array<uint8_t, 5> &bytes, int8_t &num_bytes, uint32_t num);
std::vector<uint8_t> vb_encode(const std::vector<uint32_t> &nums);
std::vector<uint32_t> vb_decode(const std::vector<uint8_t> &bytes);
void check(std::vector<uint32_t> &nums_in);
long getMemoryUsage();