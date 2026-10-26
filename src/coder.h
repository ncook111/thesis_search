#ifndef CODER_H
#define CODER_H

#include <concepts>
#include "partitioned_sequence.hpp"
#include <succinct/bit_vector.hpp>
#include "helper.h"

constexpr uint8_t BYTE_FST = 0x80; // Byte with most significant bit set
constexpr uint8_t BYTE_RST = 0x7F; // Byte with last 7 bits set
constexpr uint32_t UINT24_MAX = 0xFFFFFF;
constexpr long UINT40_MAX = 0xFFFFFFFFFF;
constexpr long UINT48_MAX = 0xFFFFFFFFFFFF;
constexpr long UINT56_MAX = 0xFFFFFFFFFFFFFF;

const std::vector<uint8_t> vb_encode_num_uint64_t(uint64_t num);
const uint64_t vb_decode_num_uint64_t(const std::vector<uint8_t> &bytes, size_t &bytes_consumed);
const std::vector<uint8_t>
gvb_encode_uint32_t(const std::vector<uint32_t> &nums);
void gvb_encode_uint32_t(const std::vector<uint32_t> &nums, std::vector<uint8_t> &encoded);
void gvb_decode_uint32_t(const std::vector<uint8_t> &bytes, std::vector<uint32_t> &decoded);
const std::vector<uint32_t>
gvb_decode_uint32_t(const std::vector<uint8_t> &bytes);
const std::vector<uint8_t>
gvb_encode_uint64_t(const std::vector<uint64_t> &nums);
const std::vector<uint64_t>
gvb_decode_uint64_t(const std::vector<uint8_t> &bytes);
inline const uint8_t get_length(const uint64_t num);
inline const uint8_t get_length(const uint32_t num);
void check(std::vector<uint32_t> &nums_in);
const long get_memory_usage();

template <class F> std::vector<F> delta_encode(const std::vector<F> &nums) {
  std::vector<F> encoded;

  F prev = 0;
  for (const F &num : nums) {
    encoded.push_back(num - prev);
    prev = num;
  }

  return encoded;
}

template <class F> std::vector<F> delta_encode(const std::vector<F> &nums, std::vector<F> &encoded) {
  F prev = 0;
  for (const F &num : nums) {
    encoded.push_back(num - prev);
    prev = num;
  }
}

template <class F> std::vector<F> delta_decode(const std::vector<F> &nums) {
  std::vector<F> decoded;

  F total = 0;
  for (const F &num : nums) {
    decoded.push_back(num + total);
    total += num;
  }

  return decoded;
}

template <class F> std::vector<F> delta_decode(const std::vector<F> &nums, const std::vector<F> &decoded) {
  F total = 0;
  for (const F &num : nums) {
    decoded.push_back(num + total);
    total += num;
  }
}

template <std::unsigned_integral T>
std::vector<uint8_t> vb_encode_num(T num) {
  std::vector<uint8_t> bytes;

  while(true) {
    bytes.push_back(num & BYTE_RST); // Get least sig 7 bits

    if (num < BYTE_FST) {
      bytes.front() |= BYTE_FST; // Set continuation bit
      break;
    }

    num >>= 7;
  }

  std::reverse(bytes.begin(), bytes.end());

  return bytes;
}

template <std::unsigned_integral T>
T vb_decode_num(const std::vector<uint8_t> &bytes, size_t &bytes_consumed) {
  T num = 0;
  bytes_consumed = 0;

  for (auto it = bytes.begin(); it != bytes.end(); ++it) {
    ++bytes_consumed;

    if (!(*it & BYTE_FST)) { // More bytes for num
      num = (num << 7) + *it;
    } else {
      num = (num << 7) + (*it & BYTE_RST); // Last byte of num
      break;
    }
  }

  return num;
}

template <class T>
  requires std::same_as<T, uint32_t> || std::same_as<T, uint64_t>
uint64_t write_gvb_to_file(const std::vector<T> &vec, std::ostream &file, const bool delta_enc = true) {
  const std::vector<uint8_t> enc = [&] {
    if constexpr (std::same_as<T, uint32_t>) {
      if (delta_enc)
        return gvb_encode_uint32_t(cvrt_1(delta_encode(vec)));
      else
        return gvb_encode_uint32_t(cvrt_1(vec));
    } else {
      if (delta_enc)
        return gvb_encode_uint64_t(cvrt_1(delta_encode(vec)));
      else
        return gvb_encode_uint64_t(cvrt_1(vec));
    }
  }();

  const uint64_t enc_size = enc.size();
  file.write(reinterpret_cast<const char *>(enc.data()), enc_size);

  return enc_size;
}

template <class T>
  requires std::same_as<T, uint32_t> || std::same_as<T, uint64_t>
std::vector<T> read_gvb_from_file(const uint8_t *file, const size_t size_to_read, const bool delta_enc = true) {
  const std::vector<uint8_t> enc(file, file + size_to_read);
  
  if constexpr (std::same_as<T, uint32_t>) {
    if (delta_enc)
      return delta_decode(cvrt_0(gvb_decode_uint32_t(enc)));
    else
      return cvrt_0(gvb_decode_uint32_t(enc));
  } else {
    if (delta_enc)
      return delta_decode(cvrt_0(gvb_decode_uint64_t(enc)));
    else
      return cvrt_0(gvb_decode_uint64_t(enc));
  }
}

#endif