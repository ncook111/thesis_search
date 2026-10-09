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

const uint16_t n_encoded_bytes_uint64_t(const uint8_t bw);
const std::vector<uint8_t> fls_encode_uint64_t(const std::vector<uint64_t> &input, std::vector<uint8_t> &output);
const FlsEncodedBlockU64 fls_encode_block_uint64_t(const std::span<const uint64_t> &input);
const std::vector<uint64_t> fls_decode_uint64_t(const std::vector<uint8_t> &input, const size_t num_blocks);
const std::vector<uint64_t> fls_decode_block_uint64_t(const FlsEncodedBlockU64 &encoded);
const FlsEncodedBlockU64 construct_encoded_block(const std::vector<uint8_t> &input, size_t &offset);
const std::vector<uint64_t> select_unffor_decoder(const std::vector<uint64_t> &encoded, const uint64_t base, const uint8_t bw);
const std::vector<uint64_t> select_delta_decoder(const std::vector<uint64_t> &encoded, const std::vector<uint64_t> &base);
uint64_t write_fls_to_file(const std::vector<uint32_t> &vec, std::ofstream &file);
std::vector<uint32_t> read_fls_from_file(const uint8_t *file, const size_t size_to_read);

#endif