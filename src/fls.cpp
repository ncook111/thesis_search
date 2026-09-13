#include <vector>
#include <span>
#include "fls_gen/transpose/transpose.hpp"
#include "fls_gen/untranspose/untranspose.hpp"
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

const std::vector<uint8_t> fls_encode_uint64_t(const std::vector<uint64_t> &input, std::vector<uint8_t> &output) {
    size_t num_blocks = 0;
    size_t i = N;
    
    for (; i <= input.size(); i += N) {
        const std::span<const uint64_t> input_view(input.data() + (i - N), N);
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

    std::vector<uint8_t> var_byte_nb = vb_encode_num_uint64_t(num_blocks);

    return var_byte_nb;
}

const FlsEncodedBlockU64 fls_encode_block_uint64_t(const std::span<const uint64_t> &input) {
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

const std::vector<uint64_t> fls_decode_uint64_t(const std::vector<uint8_t> &input, const size_t num_blocks) {
    std::vector<uint64_t> all_decoded;

    size_t offset = 0;
    for (size_t i = 0; i < num_blocks; ++i) {
        FlsEncodedBlockU64 encoded_block = construct_encoded_block(input, offset);
        std::vector<uint64_t> decoded = fls_decode_block_uint64_t(encoded_block);
        all_decoded.insert(all_decoded.end(), std::make_move_iterator(decoded.begin()), std::make_move_iterator(decoded.end()));
    }

    if (offset < input.size()) { // gvb encoded remainder
        std::vector<uint8_t> remainder(input.begin() + offset, input.end());
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
    const std::vector<uint64_t> deltas = select_unffor_decoder(encoded.encoded_data, encoded.ffor_base, encoded.bw);

    const std::vector<uint64_t> decoded = select_delta_decoder(deltas, encoded.delta_base);

    std::vector<uint64_t> untransposed(N, 0);
    generated::untranspose::fallback::scalar::untranspose_i(decoded.data(), untransposed.data());

    return untransposed;
}

const std::vector<uint64_t> select_unffor_decoder(const std::vector<uint64_t> &encoded, const uint64_t base, const uint8_t bw) {
    std::vector<uint64_t> decoded(N, 0);
    
    #if defined(SIMD_TARGET_AVX512BW)
        generated::unffor::x86_64::avx512bw::unffor(encoded.data(), decoded.data(), bw, &base);
    #elif defined(SIMD_TARGET_AVX2)
        generated::unffor::x86_64::avx2::unffor(encoded.data(), decoded.data(), bw, &base);
    #elif defined(SIMD_TARGET_SSE)
        generated::unffor::x86_64::sse::unffor(encoded.data(), decoded.data(), bw, &base);
    #elif defined(SIMD_TARGET_NEON)
        generated::unffor::arm64v8::neon::unffor(encoded.data(), decoded.data(), bw, &base);
    #elif defined(SIMD_TARGET_SCALAR)
        generated::unffor::fallback::scalar::unffor(encoded.data(), decoded.data(), bw, &base);
    #endif

    return decoded;
}

const std::vector<uint64_t> select_delta_decoder(const std::vector<uint64_t> &encoded, const std::vector<uint64_t> &base) {
   std::vector<uint64_t> decoded(N, 0);

    #if defined(SIMD_TARGET_AVX512BW)
        generated::rsum::x86_64::avx512bw::rsum(encoded.data(), decoded.data(), base.data());
    #elif defined(SIMD_TARGET_AVX2)
        generated::rsum::x86_64::avx2::rsum(encoded.data(), decoded.data(), base.data());
    #elif defined(SIMD_TARGET_SSE)
        generated::rsum::x86_64::sse::rsum(encoded.data(), decoded.data(), base.data());
    #elif defined(SIMD_TARGET_NEON)
        generated::rsum::arm64v8::neon::rsum(encoded.data(), decoded.data(), base.data());
    #elif defined(SIMD_TARGET_SCALAR)
        generated::rsum::fallback::scalar::rsum(encoded.data(), decoded.data(), base.data());
    #endif

    return decoded;
}