#include "partitioned_sequence.hpp"
#include <succinct/bit_vector.hpp>

#ifndef CODER_H
#define CODER_H

const int8_t vb_encode_num(std::array<uint8_t, 5> &bytes, uint32_t num);
const std::vector<uint8_t> vb_encode(const std::vector<uint32_t> &nums);
const std::vector<uint32_t> vb_decode(const std::vector<uint8_t> &bytes);
const std::vector<uint8_t> gvb_encode_uint32_t(const std::vector<uint32_t> &nums);
const std::vector<uint32_t> gvb_decode_uint32_t(const std::vector<uint8_t> &bytes);
const std::vector<uint8_t> gvb_encode_uint64_t(const std::vector<uint64_t> &nums);
const std::vector<uint64_t> gvb_decode_uint64_t(const std::vector<uint8_t> &bytes);
inline const uint8_t get_length(const uint64_t num);
inline const uint8_t get_length(const uint32_t num);
void check(std::vector<uint32_t> &nums_in);
const long get_memory_usage();
const std::pair<uint64_t, uint64_t> partitioned_ef_encode(succinct::bit_vector_builder &bv, const std::vector<uint64_t> &nums);
const std::vector<uint64_t> partitioned_ef_decode(const succinct::bit_vector &bv_f, const uint64_t &universe, const uint64_t &n);

template <class F>
std::vector<F> delta_encode(const std::vector<F> &nums) {
    std::vector<F> encoded;

    F prev = 0;
    for (const F &num : nums) {
        encoded.push_back(num - prev);
        prev = num;
    }

    return encoded;
}

template <class F>
std::vector<F> delta_decode(const std::vector<F> &nums) {
    std::vector<F> decoded;
   
    F total = 0;
    for (const F &num : nums) {
        decoded.push_back(num + total);
        total += num;
    }

    return decoded;
}

#endif