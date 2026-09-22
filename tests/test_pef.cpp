#include <gtest/gtest.h>
#include "../src/pef.h"
#include "fuzzing.h"

TEST(pef_encode_decode_large_number_test, test1) {
    std::vector<uint64_t> input = {1, 2, 3, 4, UINT32_MAX};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode_u64(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode_u64(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_large_number_test, test2) {
    std::vector<uint64_t> input = {1, 2, 3, 4, (uint64_t)1 << 62};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode_u64(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode_u64(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_large_number_test, test3) {
    std::vector<uint64_t> input = {1, UINT32_MAX};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode_u64(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode_u64(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_large_number_test, test4) {
    std::vector<uint64_t> input = {1, 2, UINT64_MAX-1};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode_u64(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode_u64(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_large_number_test, test5) {
    std::vector<uint64_t> input = {1, UINT32_MAX, UINT64_MAX-1};
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode_u64(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode_u64(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}

TEST(pef_encode_decode_test, test1) {
    std::vector<uint64_t> input = generate_rand_uint64_t_vec_dist(1000000);
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode_u64(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode_u64(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}