#include <gtest/gtest.h>
#include "../src/pef.h"
#include "fuzzing.h"

TEST(pef_encode_decode_u64_large_number_test, test1) {
    std::vector<uint64_t> input = {1, 2, 3, 4, UINT32_MAX};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint64_t>(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode<uint64_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u64_large_number_test, test2) {
    std::vector<uint64_t> input = {1, 2, 3, 4, (uint64_t)1 << 62};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint64_t>(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode<uint64_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u64_large_number_test, test3) {
    std::vector<uint64_t> input = {1, UINT32_MAX};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint64_t>(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode<uint64_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u64_large_number_test, test4) {
    std::vector<uint64_t> input = {1, 2, UINT64_MAX-1};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint64_t>(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode<uint64_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u64_large_number_test, test5) {
    std::vector<uint64_t> input = {1, UINT32_MAX, UINT64_MAX-1};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint64_t>(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode<uint64_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u64_test, test1) {
    std::vector<uint64_t> input = generate_rand_uint64_t_vec_dist(1000000);
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint64_t>(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode<uint64_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u32_large_number_test, test1) {
    std::vector<uint32_t> input = {1, 2, 3, 4, UINT32_MAX};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint32_t>(encoded, input);
    const std::vector<uint32_t> decoded = partitioned_ef_decode<uint32_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u32_large_number_test, test2) {
    std::vector<uint32_t> input = {1, UINT32_MAX};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint32_t>(encoded, input);
    const std::vector<uint32_t> decoded = partitioned_ef_decode<uint32_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u32_large_number_test, test3) {
    std::vector<uint32_t> input = {1, 2, UINT32_MAX-1};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint32_t>(encoded, input);
    const std::vector<uint32_t> decoded = partitioned_ef_decode<uint32_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_u32_large_number_test, test4) {
    std::vector<uint32_t> input = {1, 2, UINT32_MAX};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint32_t>(encoded, input);
    const std::vector<uint32_t> decoded = partitioned_ef_decode<uint32_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}


TEST(pef_encode_decode_u32_test, test1) {
    std::vector<uint32_t> input = generate_rand_uint32_t_vec_dist(1000000);
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode<uint32_t>(encoded, input);
    const std::vector<uint32_t> decoded = partitioned_ef_decode<uint32_t>(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}