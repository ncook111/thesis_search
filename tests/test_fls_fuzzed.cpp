#include <gtest/gtest.h>
#include "../src/fls.h"
#include "fuzzing.h"

TEST(fls_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_int_vec_dist(1000000);
    std::vector<uint8_t> encoded;
    size_t num_blocks = fls_encode_uint64_t(input, encoded);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded, num_blocks);
    std::cout << "Encoded size: " << encoded.size() << std::endl;
    std::cout << "Original size: " << input.size() * 8 << std::endl;

    ASSERT_TRUE(input == decoded);
}