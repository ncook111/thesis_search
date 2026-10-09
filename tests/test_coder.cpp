#include <gtest/gtest.h>
#include "../src/coder.h"
#include "fuzzing.h"

TEST(gvb_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_uint64_t_vec_dist(1000000);
    const std::vector<uint8_t> encoded = gvb_encode_uint64_t(input);
    const std::vector<uint64_t> decoded = gvb_decode_uint64_t(encoded);

    ASSERT_TRUE(input == decoded);
}

TEST(delta_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_uint64_t_vec_dist(1000000);
    const std::vector<uint64_t> encoded = delta_encode(input);
    const std::vector<uint64_t> decoded = delta_decode((encoded));

    ASSERT_TRUE(input == decoded);
}

TEST(gvb_delta_u64_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint64_t> input = generate_rand_uint64_t_vec_dist(1000000);
    const std::vector<uint8_t> encoded = gvb_encode_uint64_t(delta_encode(input));
    const std::vector<uint64_t> decoded = delta_decode(gvb_decode_uint64_t(encoded));

    ASSERT_TRUE(input == decoded);
}


TEST(gvb_u32_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    const std::vector<uint8_t> encoded = gvb_encode_uint32_t(input);
    const std::vector<uint32_t> decoded = gvb_decode_uint32_t(encoded);

    ASSERT_TRUE(input == decoded);
}

TEST(delta_u32_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    const std::vector<uint32_t> encoded = delta_encode(input);
    const std::vector<uint32_t> decoded = delta_decode((encoded));

    ASSERT_TRUE(input == decoded);
}

TEST(gvb_delta_u32_t_encode_decode_test_fuzzed, test1) {
    const std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    const std::vector<uint8_t> encoded = gvb_encode_uint32_t(delta_encode(input));
    const std::vector<uint32_t> decoded = delta_decode(gvb_decode_uint32_t(encoded));

    ASSERT_TRUE(input == decoded);
}

TEST(vb_encode_num_uint64_t_test, test1) {
    const uint64_t num = 0;
    const std::vector<uint8_t> encoded = vb_encode_num<uint64_t>(num);
    size_t num_consumed;
    const uint64_t decoded = vb_decode_num<uint64_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint64_t_test, test2) {
    const uint64_t num = 1;
    const std::vector<uint8_t> encoded = vb_encode_num<uint64_t>(num);
    size_t num_consumed;
    const uint64_t decoded = vb_decode_num<uint64_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint64_t_test, test3) {
    const uint64_t num = 128;
    const std::vector<uint8_t> encoded = vb_encode_num<uint64_t>(num);
    size_t num_consumed;
    const uint64_t decoded = vb_decode_num<uint64_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint64_t_test, test4) {
    const uint64_t num = UINT8_MAX;
    const std::vector<uint8_t> encoded = vb_encode_num<uint64_t>(num);
    size_t num_consumed;
    const uint64_t decoded = vb_decode_num<uint64_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint64_t_test, test5) {
    const uint64_t num = UINT16_MAX;
    const std::vector<uint8_t> encoded = vb_encode_num<uint64_t>(num);
    size_t num_consumed;
    const uint64_t decoded = vb_decode_num<uint64_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint64_t_test, test6) {
    const uint64_t num = UINT32_MAX;
    const std::vector<uint8_t> encoded = vb_encode_num<uint64_t>(num);
    size_t num_consumed;
    const uint64_t decoded = vb_decode_num<uint64_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint64_t_test, test7) {
    const uint64_t num = UINT64_MAX;
    const std::vector<uint8_t> encoded = vb_encode_num<uint64_t>(num);
    size_t num_consumed;
    const uint64_t decoded = vb_decode_num<uint64_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint64_t_fuzzed, test1) {
    const std::vector<uint64_t> input_nums = generate_rand_uint64_t_vec_dist(1000000);

    for (const auto num : input_nums) {
        const std::vector<uint8_t> encoded = vb_encode_num<uint64_t>(num);
        size_t num_consumed;
        const uint64_t decoded = vb_decode_num<uint64_t>(encoded, num_consumed);

        ASSERT_EQ(num, decoded);
    }
}

TEST(vb_encode_num_uint32_t_test, test1) {
    const uint32_t num = 0;
    const std::vector<uint8_t> encoded = vb_encode_num<uint32_t>(num);
    size_t num_consumed;
    const uint32_t decoded = vb_decode_num<uint32_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint32_t_test, test2) {
    const uint32_t num = 1;
    const std::vector<uint8_t> encoded = vb_encode_num<uint32_t>(num);
    size_t num_consumed;
    const uint32_t decoded = vb_decode_num<uint32_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint32_t_test, test3) {
    const uint32_t num = 128;
    const std::vector<uint8_t> encoded = vb_encode_num<uint32_t>(num);
    size_t num_consumed;
    const uint32_t decoded = vb_decode_num<uint32_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint32_t_test, test4) {
    const uint32_t num = UINT8_MAX;
    const std::vector<uint8_t> encoded = vb_encode_num<uint32_t>(num);
    size_t num_consumed;
    const uint32_t decoded = vb_decode_num<uint32_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint32_t_test, test5) {
    const uint32_t num = UINT16_MAX;
    const std::vector<uint8_t> encoded = vb_encode_num<uint32_t>(num);
    size_t num_consumed;
    const uint32_t decoded = vb_decode_num<uint32_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint32_t_test, test6) {
    const uint32_t num = UINT32_MAX;
    const std::vector<uint8_t> encoded = vb_encode_num<uint32_t>(num);
    size_t num_consumed;
    const uint32_t decoded = vb_decode_num<uint32_t>(encoded, num_consumed);

    ASSERT_EQ(num, decoded);
}

TEST(vb_encode_num_uint32_t_fuzzed, test1) {
    const std::vector<uint32_t> input_nums = generate_rand_uint32_t_vec_dist(1000000);

    for (const auto num : input_nums) {
        const std::vector<uint8_t> encoded = vb_encode_num<uint32_t>(num);
        size_t num_consumed;
        const uint32_t decoded = vb_decode_num<uint32_t>(encoded, num_consumed);

        ASSERT_EQ(num, decoded);
    }
}