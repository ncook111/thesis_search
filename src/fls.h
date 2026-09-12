#ifndef FLS_H
#define FLS_H

#include <vector>
#include <span>

struct FlsEncodedBlockU64 {
  uint64_t ffor_base;
  uint8_t bw;
  std::vector<uint64_t> delta_base;
  std::vector<uint64_t> encoded_data;

};

const uint8_t bits_needed(uint64_t max_val);
const uint16_t n_encoded_bytes_uint64_t(uint8_t bw);
const std::vector<uint8_t> fls_encode_uint64_t(const std::vector<uint64_t> &input);
const FlsEncodedBlockU64 fls_encode_block_uint64_t(const std::span<const uint64_t> &input);
const std::vector<uint64_t> fls_decode_uint64_t(const std::vector<uint8_t> &input);
const std::vector<uint64_t> fls_decode_block_uint64_t(const FlsEncodedBlockU64 &encoded);
const FlsEncodedBlockU64 construct_encoded_block(const std::vector<uint8_t> &input, size_t &offset);
const std::vector<uint64_t> select_unffor_decoder(const std::vector<uint64_t> &encoded, const uint64_t base, const uint8_t bw);
const std::vector<uint64_t> select_delta_decoder(const std::vector<uint64_t> &encoded, const std::vector<uint64_t> &base);

#endif