#include <vector>
#include <span>
#include "fls_gen/transpose/transpose.hpp"
#include "fls_gen/rsum_and_untranspose/rsum_and_untranspose.hpp"
#include "fls_gen/unrsum/unrsum.hpp"
#include "fls_gen/rsum/rsum.hpp"
#include "fls_gen/ffor/ffor.hpp"
#include "fls_gen/unffor/unffor.hpp"
#include "coder.h"
#include "helper.h"
#include "fls.h"

constexpr size_t N = 1024;
constexpr size_t N_LANES = 1024 / (sizeof(uint64_t) * 8);

const uint16_t n_encoded_bytes_uint64_t(uint8_t bw) {
    return bw * 128;
}

const size_t fls_encode_uint64_t(std::vector<uint64_t> &input, std::vector<uint8_t> &output) {
    size_t num_blocks = 0;
    size_t i = N;
    for (i; i <= input.size(); i += N) {
        std::span<uint64_t> input_view(input.data() + (i - N), N);
        FlsEncodedBlockU64 block = fls_encode_block_uint64_t(input_view);

        size_t old_size = output.size();
        size_t data_bytes = block.encoded_data.size() * sizeof(uint64_t);
        output.resize(old_size + 8 + 1 + N_LANES*8 + data_bytes);
        uint8_t* pos = output.data() + old_size;
        std::memcpy(pos, &block.ffor_base, 8);
        pos += 8;
        std::memcpy(pos, &block.bw, 1);
        pos += 1;
        std::memcpy(pos, block.delta_base.data(), N_LANES * 8);
        pos += N_LANES * 8;
        std::memcpy(pos, block.encoded_data.data(), data_bytes);

        ++num_blocks;
    }

    // Remaining unencoded values
    std::vector<uint8_t> remainder;
    if (i - N < input.size()) {
        size_t remaining_num = input.size() - (i - N);
        std::vector<uint64_t> input_rem_view(input.begin() + (i - N), input.begin() + (i - N) + remaining_num); // Copy can be avoided by using span, unsupported by gvb_encode_unit64_t() however
        remainder = gvb_encode_uint64_t(input_rem_view);

        output.insert(output.end(), std::make_move_iterator(remainder.begin()), std::make_move_iterator(remainder.end()));
    }

    return num_blocks;
}

const FlsEncodedBlockU64 fls_encode_block_uint64_t(std::span<uint64_t> &input) {
    FlsEncodedBlockU64 encoded;

    std::vector<uint64_t> transposed(N);
    generated::transpose::fallback::scalar::transpose_i(input.data(), transposed.data());

    encoded.delta_base = std::vector<uint64_t>(transposed.begin(), transposed.begin() + N_LANES); 

    generated::unrsum::fallback::scalar::unrsum_inplace(transposed.data());

    encoded.ffor_base = *std::min_element(transposed.begin(), transposed.end());
    encoded.bw = std::bit_width(*std::max_element(transposed.begin(), transposed.end()) - encoded.ffor_base);
    
    encoded.encoded_data.resize(n_encoded_bytes_uint64_t(encoded.bw) / 8, 0);
    generated::ffor::fallback::scalar::ffor(transposed.data(), encoded.encoded_data.data(), encoded.bw, &encoded.ffor_base);

    return encoded;
}


const std::vector<uint64_t> fls_decode_uint64_t(const std::vector<uint8_t> &input, const uint32_t num_blocks) {
    std::vector<uint64_t> all_decoded;

    size_t i = 0;
    for (size_t blk = 0; blk < num_blocks; ++blk) {
        FlsEncodedBlockU64 encoded_block = construct_encoded_block(input, i);
        std::vector<uint64_t> decoded = fls_decode_block_uint64_t(encoded_block);
        all_decoded.insert(all_decoded.end(), std::make_move_iterator(decoded.begin()), std::make_move_iterator(decoded.end()));
    }

    if (i < input.size()) { // gvb encoded remainder
        std::vector<uint8_t> remainder(input.begin() + i, input.end());
        std::vector<uint64_t> decoded = gvb_decode_uint64_t(remainder);
        all_decoded.insert(all_decoded.end(), std::make_move_iterator(decoded.begin()), std::make_move_iterator(decoded.end()));
    }

    return all_decoded;
}

const FlsEncodedBlockU64 construct_encoded_block(const std::vector<uint8_t> &input, size_t &offset) {
    FlsEncodedBlockU64 encoded_block;
    std::memcpy(&encoded_block.ffor_base, &input[offset], sizeof(uint64_t));

    offset += sizeof(uint64_t);

    std::memcpy(&encoded_block.bw, &input[offset], sizeof(uint8_t));

    offset += sizeof(uint8_t);

    encoded_block.delta_base.resize(N_LANES, 0);
    std::memcpy(encoded_block.delta_base.data(), &input[offset], N_LANES * sizeof(uint64_t));

    offset += N_LANES * sizeof(uint64_t);

    size_t n_bytes = n_encoded_bytes_uint64_t(encoded_block.bw);

    encoded_block.encoded_data.resize(n_bytes / sizeof(uint64_t), 0);
    std::memcpy(encoded_block.encoded_data.data(), &input[offset], n_bytes);

    offset += n_bytes;

    return encoded_block;
}

const std::vector<uint64_t> fls_decode_block_uint64_t(const FlsEncodedBlockU64 &encoded) {
    std::vector<uint64_t> deltas(N, 0);
    generated::unffor::fallback::scalar::unffor(encoded.encoded_data.data(), deltas.data(), encoded.bw, &encoded.ffor_base);

    std::vector<uint64_t> decoded(N, 0);
    generated::rsum_and_untranspose::fallback::scalar::rsum_and_untranspose(deltas.data(), decoded.data(), encoded.delta_base.data());

    return decoded;
}

const std::vector<uint64_t> fls_ffor_decode_scalar(const std::vector<uint64_t> &encoded, const uint64_t base, const uint8_t bw) {
    std::vector<uint64_t> decoded(n_encoded_bytes_uint64_t(bw) / sizeof(uint64_t), 0);
    generated::unffor::fallback::scalar::unffor(encoded.data(), decoded.data(), bw, &base);

    return decoded;
}

const std::vector<uint64_t> fls_ffor_decode_neon(const std::vector<uint64_t> &encoded, const uint64_t base, const uint8_t bw) {
    std::vector<uint64_t> decoded(n_encoded_bytes_uint64_t(bw) / sizeof(uint64_t), 0);
    generated::unffor::arm64v8::neon::unffor(encoded.data(), decoded.data(), bw, &base);

    return decoded;
}


const std::vector<uint64_t> fls_ffor_decode_avx512(const std::vector<uint64_t> &encoded, const uint64_t base, const uint8_t bw) {
    std::vector<uint64_t> decoded(n_encoded_bytes_uint64_t(bw) / sizeof(uint64_t), 0);
    generated::unffor::x86_64::avx512bw::unffor(encoded.data(), decoded.data(), bw, &base);

    return decoded;
}

const std::vector<uint64_t> fls_ffor_decode_avx2(const std::vector<uint64_t> &encoded, const uint64_t base, const uint8_t bw) {
    std::vector<uint64_t> decoded(n_encoded_bytes_uint64_t(bw) / sizeof(uint64_t), 0);
    generated::unffor::x86_64::avx2::unffor(encoded.data(), decoded.data(), bw, &base);

    return decoded;
}

const std::vector<uint64_t> fls_ffor_decode_sse(const std::vector<uint64_t> &encoded, const uint64_t base, const uint8_t bw) {
    std::vector<uint64_t> decoded(n_encoded_bytes_uint64_t(bw) / sizeof(uint64_t), 0);
    generated::unffor::x86_64::sse::unffor(encoded.data(), decoded.data(), bw, &base);

    return decoded;
}

const std::vector<uint64_t> fls_delta_decode_scalar(const std::vector<uint64_t> &encoded, const std::vector<uint64_t> base) {
    std::vector<uint64_t> decoded(encoded.size(), 0);
    generated::rsum::fallback::scalar::rsum(encoded.data(), decoded.data(), base.data());

    return decoded;
}

const std::vector<uint64_t> fls_delta_decode_neon(const std::vector<uint64_t> &encoded, const std::vector<uint64_t> base) {
    std::vector<uint64_t> decoded(encoded.size(), 0);
    generated::rsum::arm64v8::neon::rsum(encoded.data(), decoded.data(), base.data());

    return decoded;
}

const std::vector<uint64_t> fls_delta_decode_avx512(const std::vector<uint64_t> &encoded, const std::vector<uint64_t> base) {
    std::vector<uint64_t> decoded(encoded.size(), 0);
    generated::rsum::x86_64::avx512bw::rsum(encoded.data(), decoded.data(), base.data());

    return decoded;
}

const std::vector<uint64_t> fls_delta_decode_avx2(const std::vector<uint64_t> &encoded, const std::vector<uint64_t> base) {
    std::vector<uint64_t> decoded(encoded.size(), 0);
    generated::rsum::x86_64::avx2::rsum(encoded.data(), decoded.data(), base.data());

    return decoded;
}

const std::vector<uint64_t> fls_delta_decode_sse(const std::vector<uint64_t> &encoded, const std::vector<uint64_t> base) {
    std::vector<uint64_t> decoded(encoded.size(), 0);
    generated::rsum::x86_64::sse::rsum(encoded.data(), decoded.data(), base.data());

    return decoded;
}