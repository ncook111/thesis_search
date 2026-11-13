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
constexpr size_t N_LANES = 1024 / (sizeof(uint32_t) * 8);

const uint16_t n_encoded_bytes_uint32_t(const uint8_t bw) {
    return bw * 128;
}

const std::vector<uint8_t> fls_encode_uint32_t(const std::vector<uint32_t> &input, std::vector<uint8_t> &output) {
    size_t num_blocks = 0;
    size_t i = N;
    
    for (; i <= input.size(); i += N) {
        const std::span<const uint32_t> input_view(input.data() + (i - N), N);
        FlsEncodedBlockU32 block = fls_encode_block_uint32_t(input_view);

        size_t old_size = output.size();
        size_t data_bytes = block.encoded_data.size() * sizeof(uint32_t);
        output.resize(old_size + sizeof(uint32_t) + 1 + N_LANES * sizeof(uint32_t) + data_bytes);
        uint8_t* pos = output.data() + old_size;
        std::memcpy(pos, &block.ffor_base, sizeof(uint32_t));
        pos += sizeof(uint32_t);
        std::memcpy(pos, &block.bw, 1);
        pos += 1;
        std::memcpy(pos, block.delta_base.data(), N_LANES * sizeof(uint32_t));
        pos += N_LANES * sizeof(uint32_t);
        std::memcpy(pos, block.encoded_data.data(), data_bytes);

        ++num_blocks;
    }

    // Remaining unencoded values
    std::vector<uint8_t> remainder;
    if (i - N < input.size()) {
        size_t remaining_num = input.size() - (i - N);
        std::vector<uint32_t> input_rem_view(input.begin() + (i - N), input.begin() + (i - N) + remaining_num); // Copy can be avoided by using span, unsupported by gvb_encode_uint32_t() however
        remainder = gvb_encode_uint32_t(cvrt_1(std::move(input_rem_view)));

        output.insert(output.end(), std::make_move_iterator(remainder.begin()), std::make_move_iterator(remainder.end()));
    }

    std::vector<uint8_t> var_byte_nb = vb_encode_num<uint64_t>(num_blocks);

    return var_byte_nb;
}

const FlsEncodedBlockU32 fls_encode_block_uint32_t(const std::span<const uint32_t> &input) {
    FlsEncodedBlockU32 encoded;

    std::vector<uint32_t> transposed(N);
    generated::transpose::fallback::scalar::transpose_i(input.data(), transposed.data());

    encoded.delta_base = std::vector<uint32_t>(transposed.begin(), transposed.begin() + N_LANES); 

    generated::unrsum::fallback::scalar::unrsum_inplace(transposed.data());

    encoded.ffor_base = *std::min_element(transposed.begin(), transposed.end());
    encoded.bw = std::bit_width(*std::max_element(transposed.begin(), transposed.end()) - encoded.ffor_base);
    
    encoded.encoded_data.resize(n_encoded_bytes_uint32_t(encoded.bw) / sizeof(uint32_t), 0);
    generated::ffor::fallback::scalar::ffor(transposed.data(), encoded.encoded_data.data(), encoded.bw, &encoded.ffor_base);

    return encoded;
}

const std::vector<uint32_t> fls_decode_uint32_t(const std::vector<uint8_t> &input, const size_t num_blocks) {
    std::vector<uint32_t> all_decoded;

    size_t offset = 0;
    for (size_t i = 0; i < num_blocks; ++i) {
        FlsEncodedBlockU32 encoded_block = construct_encoded_block(input, offset);
        std::vector<uint32_t> decoded = fls_decode_block_uint32_t(encoded_block);
        all_decoded.insert(all_decoded.end(), std::make_move_iterator(decoded.begin()), std::make_move_iterator(decoded.end()));
    }

    if (offset < input.size()) { // gvb encoded remainder
        std::vector<uint8_t> remainder(input.begin() + offset, input.end());
        std::vector<uint32_t> decoded = cvrt_0(gvb_decode_uint32_t(remainder));
        all_decoded.insert(all_decoded.end(), std::make_move_iterator(decoded.begin()), std::make_move_iterator(decoded.end()));
    }

    return all_decoded;
}

const FlsEncodedBlockU32 construct_encoded_block(const std::vector<uint8_t> &input, size_t &offset) {
    FlsEncodedBlockU32 encoded_block;
    std::memcpy(&encoded_block.ffor_base, &input[offset], sizeof(uint32_t));

    offset += sizeof(uint32_t);

    std::memcpy(&encoded_block.bw, &input[offset], sizeof(uint8_t));

    offset += sizeof(uint8_t);

    encoded_block.delta_base.resize(N_LANES, 0);
    std::memcpy(encoded_block.delta_base.data(), &input[offset], N_LANES * sizeof(uint32_t));

    offset += N_LANES * sizeof(uint32_t);

    size_t n_bytes = n_encoded_bytes_uint32_t(encoded_block.bw);

    encoded_block.encoded_data.resize(n_bytes / sizeof(uint32_t), 0);
    std::memcpy(encoded_block.encoded_data.data(), &input[offset], n_bytes);

    offset += n_bytes;

    return encoded_block;
}

const std::vector<uint32_t> fls_decode_block_uint32_t(const FlsEncodedBlockU32 &encoded) {
    const std::vector<uint32_t> deltas = select_unffor_decoder(encoded.encoded_data, encoded.ffor_base, encoded.bw);

    const std::vector<uint32_t> decoded = select_delta_decoder(deltas, encoded.delta_base);

    std::vector<uint32_t> untransposed(N, 0);
    generated::untranspose::fallback::scalar::untranspose_i(decoded.data(), untransposed.data());

    return untransposed;
}

const std::vector<uint32_t> select_unffor_decoder(const std::vector<uint32_t> &encoded, const uint32_t base, const uint8_t bw) {
    std::vector<uint32_t> decoded(N, 0);
    
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

const std::vector<uint32_t> select_delta_decoder(const std::vector<uint32_t> &encoded, const std::vector<uint32_t> &base) {
   std::vector<uint32_t> decoded(N, 0);

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

uint64_t write_fls_to_file(const std::vector<uint32_t> &vec, std::ostream &file) {
  std::vector<uint8_t> enc_data;
  std::vector<uint8_t> enc_num_blocks = fls_encode_uint32_t(vec, enc_data);
  const uint64_t enc_data_size = enc_data.size();
  const uint64_t enc_num_blocks_size = enc_num_blocks.size();
  
  file.write(reinterpret_cast<const char *>(enc_num_blocks.data()), enc_num_blocks.size());
  file.write(reinterpret_cast<const char *>(enc_data.data()), enc_data.size());

  return enc_data_size + enc_num_blocks_size;
}

std::vector<uint32_t> read_fls_from_file(const uint8_t *file, const size_t size_to_read) {
  size_t enb_size_to_read = std::min({size_to_read, (size_t)10}); // num_blocks can be up to 10 bytes, get lesser of size_to_read or 10
  std::vector<uint8_t> enc_num_blocks(file, file + enb_size_to_read); 
  size_t num_bytes;
  size_t num_blocks = vb_decode_num<uint64_t>(enc_num_blocks, num_bytes);

  std::vector<uint8_t> encoded_fls(size_to_read - num_bytes);

  std::memcpy(reinterpret_cast<char *>(encoded_fls.data()), file + num_bytes, size_to_read - num_bytes);

  return fls_decode_uint32_t(encoded_fls, num_blocks);
}

#if defined(SIMD_TARGET_AVX512BW)
void fls_delta_decode_avx512(const std::vector<uint32_t> &base, const std::vector<uint32_t> &input, std::vector<uint32_t> &decoded) {
    const size_t num_blocks = input.size() / N;
    decoded.resize(input.size());
    uint32_t buf[N];
    for (size_t b = 0; b < num_blocks; ++b) {
        const size_t off = b * N;
        generated::rsum::x86_64::avx512bw::rsum(input.data() + off, buf, base.data() + b * N_LANES);
        generated::untranspose::fallback::scalar::untranspose_i(buf, decoded.data() + off);
    }
}
#endif

#if defined(SIMD_TARGET_AVX2)
void fls_delta_decode_avx2(const std::vector<uint32_t> &base, const std::vector<uint32_t> &input, std::vector<uint32_t> &decoded) {
    const size_t num_blocks = input.size() / N;
    decoded.resize(input.size());
    uint32_t buf[N];
    for (size_t b = 0; b < num_blocks; ++b) {
        const size_t off = b * N;
        generated::rsum::x86_64::avx2::rsum(input.data() + off, buf, base.data() + b * N_LANES);
        generated::untranspose::fallback::scalar::untranspose_i(buf, decoded.data() + off);
    }
}
#endif

#if defined(SIMD_TARGET_SSE)
void fls_delta_decode_sse(const std::vector<uint32_t> &base, const std::vector<uint32_t> &input, std::vector<uint32_t> &decoded) {
    const size_t num_blocks = input.size() / N;
    decoded.resize(input.size());
    uint32_t buf[N];
    for (size_t b = 0; b < num_blocks; ++b) {
        const size_t off = b * N;
        generated::rsum::x86_64::sse::rsum(input.data() + off, buf, base.data() + b * N_LANES);
        generated::untranspose::fallback::scalar::untranspose_i(buf, decoded.data() + off);
    }
}
#endif
#endif

void fls_delta_decode_scalar(const std::vector<uint32_t> &base, const std::vector<uint32_t> &input, std::vector<uint32_t> &decoded) {
    const size_t num_blocks = input.size() / N;
    decoded.resize(input.size());
    uint32_t buf[N];
    for (size_t b = 0; b < num_blocks; ++b) {
        const size_t off = b * N;
        generated::rsum::fallback::scalar::rsum(input.data() + off, buf, base.data() + b * N_LANES);
        generated::untranspose::fallback::scalar::untranspose_i(buf, decoded.data() + off);
    }
}

void fls_delta_encode_scalar(std::vector<uint32_t> &input, std::vector<uint32_t> &encoded, std::vector<uint32_t> &base) {
    size_t num_blocks;

    // pad tail up to a whole block by repeating last value.
    num_blocks = (input.size() + N-1) / N;
    input.resize(num_blocks * N, input.back());

    encoded.resize(input.size());
    base.resize(num_blocks * N_LANES);
    for (size_t b = 0; b < num_blocks; ++b) {
        uint32_t* block = encoded.data() + b * N;
        generated::transpose::fallback::scalar::transpose_i(input.data() + b * N, block);
        std::copy(block, block + N_LANES, base.begin() + b * N_LANES);
        generated::unrsum::fallback::scalar::unrsum_inplace(block);
    }
}