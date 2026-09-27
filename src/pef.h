#ifndef PEF_H
#define PEF_H

#include "helper.h"
#include "partitioned_sequence.hpp"
#include <succinct/bit_vector.hpp>
#include <utility>
#include <optional>

struct Range {
  int64_t min;
  int64_t max;
};

struct Pef_Header {
  uint64_t num_bits;
  uint64_t universe;
  uint64_t n;
};

const Pef_Header
partitioned_ef_encode_u32(succinct::bit_vector &bv,
                      const std::vector<uint32_t> &nums);
const std::vector<uint32_t>
partitioned_ef_decode_u32(const succinct::bit_vector &bv_f, const uint64_t universe,
                      const uint64_t n);
const Pef_Header
partitioned_ef_encode_u64(succinct::bit_vector &bv,
                      const std::vector<uint64_t> &nums);
const std::vector<uint64_t>
partitioned_ef_decode_u64(const succinct::bit_vector &bv_f, const uint64_t universe,
                      const uint64_t n);
const std::optional<uint64_t> find_pef_id(const std::string &file_path,
                                          const uint64_t offset,
                                          const uint64_t size_to_read,
                                          const uint64_t id);
const uint8_t write_bv_header(const Pef_Header header, std::ofstream &file);
const std::pair<Pef_Header, uint8_t> read_bv_header(const uint8_t *file);
const succinct::bit_vector get_bv(const uint8_t *file, const Pef_Header &header, const uint64_t size_to_read);
const std::optional<Range> find_range_pef_id(const std::string &file_path,
                                             const uint64_t offset,
                                             const uint64_t size_to_read,
                                             const Range range);
uint64_t write_pef_to_file(const std::vector<uint64_t> &vec, std::ofstream &file);
std::vector<uint64_t> read_pef_from_file(const uint8_t *file, const size_t size_to_read);

#endif