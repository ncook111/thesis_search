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

const std::optional<uint64_t> find_pef_id(const std::string &file_path,
                                          const uint64_t offset,
                                          const uint64_t size_to_read,
                                          const uint64_t id);
const uint8_t write_bv_header(const Pef_Header header, std::ostream &file);
const std::pair<Pef_Header, uint8_t> read_bv_header(const uint8_t *file);
const succinct::bit_vector get_bv(const uint8_t *file, const Pef_Header &header, const uint64_t size_to_read);
const std::optional<Range> find_range_pef_id(const std::string &file_path,
                                             const uint64_t offset,
                                             const uint64_t size_to_read,
                                             const Range range);

template <std::unsigned_integral T>
Pef_Header partitioned_ef_encode(succinct::bit_vector &bv,
                      const std::vector<T> &nums) {
  const uint64_t n = nums.size();
  const uint64_t universe = n ? uint64_t(nums.back()) + 1 : 0;
  
  // Range check values for failure conditions
  if constexpr (sizeof(T) == sizeof(uint64_t)) {
    if (n && nums.back() == UINT64_MAX)
      throw std::out_of_range("PEF: Value will cause universe out of bounds");
  }

  succinct::bit_vector_builder bvb;

  if (nums.size())
    ds2i::partitioned_sequence<>::write(bvb, nums.begin(), universe, n, ds2i::global_parameters());

  succinct::bit_vector tmp(&bvb);
  bv.swap(tmp);

  const uint64_t num_bits = bv.size();

  return Pef_Header(num_bits, universe, n);
}

template <std::unsigned_integral T>
std::vector<T> partitioned_ef_decode(const succinct::bit_vector &bv_f, const uint64_t universe,
                      const uint64_t n) {
  if (n == 0) return {};

  ds2i::partitioned_sequence<>::enumerator e(bv_f, 0, universe, n, ds2i::global_parameters());

  std::vector<T> decoded(n);
  decoded[0] = e.move(0).second;
  for (uint64_t i = 1; i < n; ++i) {
    decoded[i] = e.next().second;
  }

  return decoded;
}

template <std::unsigned_integral T>
uint64_t write_pef_to_file(const std::vector<T> &vec, std::ostream &file) {
  succinct::bit_vector enc;
  const Pef_Header header = partitioned_ef_encode<T>(enc, vec);
  
  const size_t c_header_size = write_bv_header(header, file);

  // Write id_to_offsets data
  uint64_t enc_size = enc.data().size() * sizeof(uint64_t);
  file.write(reinterpret_cast<const char *>(enc.data().data()), enc_size);

  return enc_size + c_header_size;
}

template <std::unsigned_integral T>
std::vector<T> read_pef_from_file(const uint8_t *file, const size_t size_to_read) {
  // Read in universe and n first
  const auto  &[header, size] = read_bv_header(file);
  const succinct::bit_vector bv =
      get_bv(file + size, header, size_to_read - size);

  return partitioned_ef_decode<T>(bv, header.universe, header.n);
}

#endif