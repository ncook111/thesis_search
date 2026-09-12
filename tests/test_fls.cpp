#include <gtest/gtest.h>
#include "../src/fls.h"
#include "fuzzing.h"

TEST(fls_u64_t_encode_decode_test_small, test1) {
    const std::vector<uint64_t> input = {1};
    std::vector<uint8_t> encoded = fls_encode_uint64_t(input);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded);

    ASSERT_TRUE(input == decoded);
}

TEST(fls_u64_t_encode_decode_test_small, test2) {
    const std::vector<uint64_t> input = {555};
    std::vector<uint8_t> encoded = fls_encode_uint64_t(input);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded);

    ASSERT_TRUE(input == decoded);
}

TEST(fls_u64_t_encode_decode_test_small, test3) {
    const std::vector<uint64_t> input = {UINT64_MAX};
    std::vector<uint8_t> encoded = fls_encode_uint64_t(input);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded);

    ASSERT_TRUE(input == decoded);
}

TEST(fls_u64_t_encode_decode_test_small, test4) {
    const std::vector<uint64_t> input = {1, 2, 3, 4, 5, 6};
    std::vector<uint8_t> encoded = fls_encode_uint64_t(input);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded);

    ASSERT_TRUE(input == decoded);
}

TEST(fls_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_int_vec_dist(1000000);
    std::vector<uint8_t> encoded = fls_encode_uint64_t(input);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded);

    ASSERT_TRUE(input == decoded);
}