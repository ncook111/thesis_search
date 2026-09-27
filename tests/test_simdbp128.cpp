#include <gtest/gtest.h>
#include "../src/simdbp128.h"
#include "../src/helper.h"
#include "fuzzing.h"

TEST(simpbp128d1_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPD1);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPD1, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(simpbp128d2_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPD2);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPD2, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(simpbp128d4_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPD4);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPD4, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(simpbp128dm_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(simpbp128dm_encode_decode_test_small, test1) {
    const std::vector<uint32_t> input = {0};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(simpbp128dm_encode_decode_test_small, test2) {
    const std::vector<uint32_t> input = {1};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(simpbp128dm_encode_decode_test_small, test3) {
    const std::vector<uint32_t> input = {1, 2, 3, 4, 5, 6};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}