#ifndef PEF_H
#define PEF_H

#include "helper.h"
#include "partitioned_sequence.hpp"
#include "pef.h"
#include <succinct/bit_vector.hpp>
#include <utility>

struct Range {
  int64_t min;
  int64_t max;
};

struct Pef_Header {
  uint64_t num_bits;
  uint64_t universe;
  uint64_t n;
};

const std::pair<uint64_t, uint64_t>
partitioned_ef_encode(succinct::bit_vector &bv,
                      const std::vector<uint64_t> &nums);
const std::vector<uint64_t>
partitioned_ef_decode(const succinct::bit_vector &bv_f, const uint64_t universe,
                      const uint64_t n);
const std::optional<uint64_t> find_pef_id(const std::string &file_path,
                                          const uint64_t offset,
                                          const uint64_t size_to_read,
                                          const uint64_t id);
const Pef_Header get_bv_header(const uint8_t *file, const uint64_t offset);
const succinct::bit_vector get_bv(const uint8_t *file, const Pef_Header &header,
                                  const uint64_t offset,
                                  const uint64_t size_to_read);
const std::optional<Range> find_range_pef_id(const std::string &file_path,
                                             const uint64_t offset,
                                             const uint64_t size_to_read,
                                             const Range range);

#endif