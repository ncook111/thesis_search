#include <gtest/gtest.h>
#include "../src/pef.h"
#include "fuzzing.h"

TEST(pef_encode_decode_test, test1) {
    std::vector<uint64_t> input = generate_rand_int_vec_dist(1000000);
    succinct::bit_vector encoded;
    const Pef_Header header = partitioned_ef_encode(encoded, input);
    const std::vector<uint64_t> decoded = partitioned_ef_decode(encoded, header.universe, header.n);

    ASSERT_TRUE(input == decoded);
}