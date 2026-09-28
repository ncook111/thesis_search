#include <gtest/gtest.h>
#include "../src/simdbp128.h"
#include "../src/helper.h"
#include "fuzzing.h"

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
TEST(s4_bpd1_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPD1);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPD1, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s4_bpd2_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPD2);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPD2, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s4_bpd4_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPD4);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPD4, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s4_bpdm_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s4_bpdm_encode_decode_test_small, test1) {
    const std::vector<uint32_t> input = {0};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s4_bpdm_encode_decode_test_small, test2) {
    const std::vector<uint32_t> input = {1};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s4_bpdm_encode_decode_test_small, test3) {
    const std::vector<uint32_t> input = {1, 2, 3, 4, 5, 6};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S4_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S4_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}
#endif

#if defined(SIMD_TARGET_AVX2)
TEST(s8_bpd4_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S8_BPD4);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S8_BPD4, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s8_bpdm_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S8_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S8_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s8_bpdm_encode_decode_test_small, test1) {
    const std::vector<uint32_t> input = {0};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S8_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S8_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s8_bpdm_encode_decode_test_small, test2) {
    const std::vector<uint32_t> input = {1};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S8_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S8_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s8_bpdm_encode_decode_test_small, test3) {
    const std::vector<uint32_t> input = {1, 2, 3, 4, 5, 6};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S8_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S8_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}
#endif

#if defined(SIMD_TARGET_AVX2)
TEST(s16_bpd4_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S16_BPD4);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S16_BPD4, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s16_bpdm_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S16_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S16_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s16_bpdm_encode_decode_test_small, test1) {
    const std::vector<uint32_t> input = {0};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S16_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S16_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s16_bpdm_encode_decode_test_small, test2) {
    const std::vector<uint32_t> input = {1};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S16_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S16_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}

TEST(s16_bpdm_encode_decode_test_small, test3) {
    const std::vector<uint32_t> input = {1, 2, 3, 4, 5, 6};
    std::vector<uint32_t> input_copy(input);
    std::vector<uint32_t> encoded = encode_simdbp128(input_copy, EncType::S16_BPDM);
    const std::vector<uint32_t> decoded = decode_simdbp128(encoded, EncType::S16_BPDM, input.size());

    ASSERT_TRUE(input == decoded);
}
#endif