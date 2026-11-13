#ifndef FLS_H
#define FLS_H

#include <vector>
#include <span>

struct FlsEncodedBlockU32 {
  uint32_t ffor_base;
  uint8_t bw;
  std::vector<uint32_t> delta_base;
  std::vector<uint32_t> encoded_data;

};

const uint16_t n_encoded_bytes_uint32_t(const uint8_t bw);
const std::vector<uint8_t> fls_encode_uint32_t(const std::vector<uint32_t> &input, std::vector<uint8_t> &output);
const FlsEncodedBlockU32 fls_encode_block_uint32_t(const std::span<const uint32_t> &input);
const std::vector<uint32_t> fls_decode_uint32_t(const std::vector<uint8_t> &input, const size_t num_blocks);
const std::vector<uint32_t> fls_decode_block_uint32_t(const FlsEncodedBlockU32 &encoded);
const FlsEncodedBlockU32 construct_encoded_block(const std::vector<uint8_t> &input, size_t &offset);
const std::vector<uint32_t> select_unffor_decoder(const std::vector<uint32_t> &encoded, const uint32_t base, const uint8_t bw);
const std::vector<uint32_t> select_delta_decoder(const std::vector<uint32_t> &encoded, const std::vector<uint32_t> &base);
uint64_t write_fls_to_file(const std::vector<uint32_t> &vec, std::ostream &file);
std::vector<uint32_t> read_fls_from_file(const uint8_t *file, const size_t size_to_read);
void fls_delta_decode_avx512(const std::vector<uint32_t> &base, const std::vector<uint32_t> &input, std::vector<uint32_t> &decoded);
void fls_delta_decode_avx2(const std::vector<uint32_t> &base, const std::vector<uint32_t> &input, std::vector<uint32_t> &decoded);
void fls_delta_decode_sse(const std::vector<uint32_t> &base, const std::vector<uint32_t> &input, std::vector<uint32_t> &decoded);
void fls_delta_decode_scalar(const std::vector<uint32_t> &base, const std::vector<uint32_t> &input, std::vector<uint32_t> &decoded);
void fls_delta_encode_scalar(std::vector<uint32_t> &input, std::vector<uint32_t> &encoded, std::vector<uint32_t> &base);
#endif