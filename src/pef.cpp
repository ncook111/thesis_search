#include "pef.h"
#include "helper.h"
#include "coder.h"
#include "partitioned_sequence.hpp"
#include <optional>
#include <succinct/bit_vector.hpp>
#include <utility>

const std::optional<Range> find_range_pef_id(const std::string &file_path,
                                             const uint64_t offset,
                                             const uint64_t size_to_read,
                                             const Range range) {
  const MappedFile mf = open_mmap_r(file_path);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();

  // Read in header
  const auto  &[header, size] = read_bv_header(file + offset);

  if (!header.n)
    return std::nullopt;
  
  const succinct::bit_vector bv = get_bv(file + offset + size, header, size_to_read - size);

  ds2i::partitioned_sequence<>::enumerator e(bv, 0, header.universe, header.n,
                                             ds2i::global_parameters());

  const auto /*uint64_t*/ [pef_id_min, value_min] = e.next_geq(range.min);
  auto /*uint64_t*/ [pef_id_max, value_max] = e.next_geq(range.max);

  if (pef_id_min == header.n)
    return std::nullopt;
  if (pef_id_max == header.n)
    --pef_id_max;

  return std::make_optional(Range(static_cast<int64_t>(pef_id_min),
                                  static_cast<int64_t>(pef_id_max)));
}

const std::optional<uint64_t> find_pef_id(const std::string &file_path,
                                          const uint64_t offset,
                                          const uint64_t size_to_read,
                                          const uint64_t id) {
  const MappedFile mf = open_mmap_r(file_path);
  const uint8_t *file = static_cast<uint8_t *>(mf.addr());
  const size_t file_size = mf.size();

  // Read in header
  const auto  &[header, size] = read_bv_header(file + offset);

  if (!header.n)
    return std::nullopt;
  
  const succinct::bit_vector bv = get_bv(file + offset + size, header, size_to_read - size);
  ds2i::partitioned_sequence<>::enumerator e(bv, 0, header.universe, header.n,
                                             ds2i::global_parameters());

  const auto [pef_id, value] = e.next_geq(id);

  return (value == id && value != header.universe) ? std::make_optional(pef_id) : std::nullopt;
}

const uint8_t write_bv_header(const Pef_Header header, std::ostream &file) {
  const std::vector<uint8_t> compressed = gvb_encode_uint64_t({header.num_bits+1, header.universe+1, header.n+1});

  // Write size of following compressed header
  const uint8_t size_byte = static_cast<uint8_t>(compressed.size());
  file.write(reinterpret_cast<const char *>(&size_byte), sizeof(size_byte));

  // Write num_bits, universe and n
  file.write(reinterpret_cast<const char *>(compressed.data()), compressed.size());

  return compressed.size() + 1;
}

const std::pair<Pef_Header,uint8_t> read_bv_header(const uint8_t *file) {
  // Get size of encoded header
  uint8_t size;
  std::memcpy(reinterpret_cast<uint8_t *>(&size), file, sizeof(uint8_t));

  std::vector<uint8_t> encoded;
  encoded.resize(size);

  std::memcpy(encoded.data(), file + sizeof(uint8_t), size);

  std::vector<uint64_t> decoded = gvb_decode_uint64_t(encoded);
  
  Pef_Header header = {decoded[0]-1, decoded[1]-1, decoded[2]-1};

  return std::make_pair(header, size + 1);
}

const succinct::bit_vector get_bv(const uint8_t *file, const Pef_Header &header, const uint64_t size_to_read) {
  const uint64_t *raw_input = reinterpret_cast<const uint64_t *>(file);
  const uint64_t num_words = size_to_read / 8;

  succinct::bit_vector_builder bvb2(header.num_bits);
  succinct::bit_vector_builder::bits_type &target = bvb2.move_bits();

  std::copy(raw_input, raw_input + num_words, target.begin());

  return succinct::bit_vector(&bvb2);
}