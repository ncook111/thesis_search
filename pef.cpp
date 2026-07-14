#include "helper.h"
#include "pef.h"
#include "partitioned_sequence.hpp"
#include <succinct/bit_vector.hpp>
#include <optional>
#include <utility>

// Encoding a single vector with ef
const std::pair<uint64_t, uint64_t> partitioned_ef_encode(succinct::bit_vector &bv, const std::vector<uint64_t> &nums) {
    const uint64_t universe = nums.back() + 1;
    const uint64_t n = nums.size();
    succinct::bit_vector_builder bvb;

    if (nums.size())
        ds2i::partitioned_sequence<>::write(bvb, nums.begin(), universe, n, ds2i::global_parameters()); 

    succinct::bit_vector tmp(&bvb);
    bv.swap(tmp);

    return std::make_pair(universe, n);
}

const std::vector<uint64_t> partitioned_ef_decode(const succinct::bit_vector &bv_f, const uint64_t universe, const uint64_t n) {
    std::vector<uint64_t> decoded;

    ds2i::partitioned_sequence<>::enumerator e(bv_f, 0, universe, n, ds2i::global_parameters());

    decoded.reserve(n);
    for (uint64_t i = 0; i < n; ++i) {
        decoded.push_back(e.move(i).second);
    }

    return decoded;
}

const std::optional<Range> find_range_pef_id(const std::string &file_path, const uint64_t offset, const uint64_t size_to_read, const Range range) {
    const auto [min, max] = range;
    
    const MappedFile mf = open_mmap_r(file_path);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();

    // Read in header
    Pef_Header header = get_bv_header(file, offset);

    // If null return empty vector
    if (!header.n) return std::nullopt;

    const succinct::bit_vector bv = get_bv(file, header, offset + sizeof(header), size_to_read - sizeof(header));

    ds2i::global_parameters params;
    ds2i::partitioned_sequence<>::enumerator e(bv, 0, header.universe, header.n, params);

    Range ids_range = {};
    
    uint64_t cur = min;
    auto /*uint64_t*/ [pef_id, value] = e.next_geq(cur);

    if (value > max)
        return std::nullopt;

    ids_range.min = pef_id;
    ids_range.max = pef_id;
    cur = value + 1;

    while (cur <= max) {
        std::tie(pef_id, value) = e.next_geq(cur);

        if (value <= max)
            ids_range.max = pef_id;

        if (pef_id >= header.n - 1) {
            break;
        } else {
            cur = value + 1;
        }
    }

    return std::make_optional(ids_range);
}

const std::optional<uint64_t> find_pef_id(const std::string &file_path, const uint64_t offset, const uint64_t size_to_read, const uint64_t id) {
    const MappedFile mf = open_mmap_r(file_path);
    const uint8_t *file = static_cast<uint8_t*>(mf.addr());
    const size_t file_size = mf.size();

    // Read in header
    Pef_Header header = get_bv_header(file, offset);

    if (!header.n) return std::nullopt;

    const succinct::bit_vector bv = get_bv(file, header, offset + sizeof(header), size_to_read - sizeof(header));

    ds2i::global_parameters params;
    ds2i::partitioned_sequence<>::enumerator e(bv, 0, header.universe, header.n, params);

    const auto /*uint64_t*/ [pef_id, value] = e.next_geq(id);

    return (value == id) ? std::make_optional(pef_id) : std::nullopt;
}

const Pef_Header get_bv_header(const uint8_t *file, const uint64_t offset) {
    Pef_Header header = {};
    std::memcpy(reinterpret_cast<char*>(&header), &file[offset], sizeof(header));

    return header;
}

const succinct::bit_vector get_bv(const uint8_t *file, const Pef_Header &header, const uint64_t offset, const uint64_t size_to_read) {
    const uint64_t* raw_input = reinterpret_cast<const uint64_t*>(&file[offset]);
    const uint64_t num_words = size_to_read / 8;

    succinct::bit_vector_builder bvb2(header.num_bits);
    succinct::bit_vector_builder::bits_type &target = bvb2.move_bits(); 

    std::copy(raw_input, raw_input + num_words, target.begin());

    return succinct::bit_vector(&bvb2);
}