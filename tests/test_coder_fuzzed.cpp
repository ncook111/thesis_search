#include <gtest/gtest.h>
#include "../src/coder.h"
#include "fuzzing.h"

TEST(gvb_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_int_vec_dist(1000000);
    const std::vector<uint8_t> encoded = gvb_encode_uint64_t(input);
    const std::vector<uint64_t> decoded = gvb_decode_uint64_t(encoded);

    ASSERT_TRUE(input == decoded);
}

TEST(delta_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_int_vec_dist(1000000);
    const std::vector<uint64_t> encoded = delta_encode(input);
    const std::vector<uint64_t> decoded = delta_decode((encoded));

    ASSERT_TRUE(input == decoded);
}

TEST(gvb_delta_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_int_vec_dist(1000000);
    const std::vector<uint8_t> encoded = gvb_encode_uint64_t(delta_encode(input));
    const std::vector<uint64_t> decoded = delta_decode(gvb_decode_uint64_t(encoded));

    ASSERT_TRUE(input == decoded);
}