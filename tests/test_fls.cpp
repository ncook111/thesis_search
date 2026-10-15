#include <gtest/gtest.h>
#include "../src/fls.h"
#include "../src/coder.h"
#include "fuzzing.h"

TEST(fls_u64_t_encode_decode_test_small, test1) {
    const std::vector<uint64_t> input = {1};
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> enc_num_blocks = fls_encode_uint64_t(input, encoded);
    size_t num_consumed;
    size_t num_blocks = vb_decode_num<uint64_t>(enc_num_blocks, num_consumed);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded, num_blocks);

    ASSERT_TRUE(input == decoded);
}

TEST(fls_u64_t_encode_decode_test_small, test2) {
    const std::vector<uint64_t> input = {555};
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> enc_num_blocks = fls_encode_uint64_t(input, encoded);
    size_t num_consumed;
    size_t num_blocks = vb_decode_num<uint64_t>(enc_num_blocks, num_consumed);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded, num_blocks);

    ASSERT_TRUE(input == decoded);
}

TEST(fls_u64_t_encode_decode_test_small, test4) {
    const std::vector<uint64_t> input = {1, 2, 3, 4, 5, 6};
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> enc_num_blocks = fls_encode_uint64_t(input, encoded);
    size_t num_consumed;
    size_t num_blocks = vb_decode_num<uint64_t>(enc_num_blocks, num_consumed);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded, num_blocks);

    ASSERT_TRUE(input == decoded);
}

TEST(fls_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_uint64_t_vec_dist(1000000);
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> enc_num_blocks = fls_encode_uint64_t(input, encoded);
    size_t num_consumed;
    size_t num_blocks = vb_decode_num<uint64_t>(enc_num_blocks, num_consumed);
    const std::vector<uint64_t> decoded = fls_decode_uint64_t(encoded, num_blocks);

    ASSERT_TRUE(input == decoded);
}