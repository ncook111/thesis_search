#include "coder.h"
#include "partitioned_sequence.hpp"
#include <iostream>
#include <random>
#include <succinct/bit_vector.hpp>
#include <sys/resource.h>

const long get_memory_usage() {
  struct rusage usage;
  if (0 == getrusage(RUSAGE_SELF, &usage))
    return usage.ru_maxrss; // bytes
  else
    return 0;
}

inline const uint8_t get_length(const uint32_t num) {
  if (num <= UINT8_MAX) {
    return 0;
  } else if (num <= UINT16_MAX) {
    return 1;
  } else if (num <= UINT24_MAX) {
    return 2;
  } else {
    return 3;
  }
}

inline const uint8_t get_length(const uint64_t num) {
  if (num <= UINT8_MAX) {
    return 0;
  } else if (num <= UINT16_MAX) {
    return 1;
  } else if (num <= UINT24_MAX) {
    return 2;
  } else if (num <= UINT32_MAX) {
    return 3;
  } else if (num <= UINT40_MAX) {
    return 4;
  } else if (num <= UINT48_MAX) {
    return 5;
  } else if (num <= UINT56_MAX) {
    return 6;
  } else {
    return 7;
  }
}

static inline void put_value(std::vector<uint8_t> &out, const uint32_t v, const uint8_t len) {
  switch (len) {
    case 3: out.push_back(v >> 24);
    [[fallthrough]];
    case 2: out.push_back(v >> 16);
    [[fallthrough]];
    case 1: out.push_back(v >> 8); 
    [[fallthrough]];
    case 0: out.push_back(v);
  }
}

static inline void put_value(std::vector<uint8_t> &out, const uint64_t v, const uint8_t len) {
  switch (len) {
    case 7:
      out.emplace_back(v >> 56);
      [[fallthrough]];
    case 6:
      out.emplace_back(v >> 48);
      [[fallthrough]];
    case 5:
      out.emplace_back(v >> 40);
      [[fallthrough]];
    case 4:
      out.emplace_back(v >> 32);
      [[fallthrough]];
    case 3:
      out.emplace_back(v >> 24);
      [[fallthrough]];
    case 2:
      out.emplace_back(v >> 16);
      [[fallthrough]];
    case 1:
      out.emplace_back(v >> 8);
      [[fallthrough]];
    case 0:
      out.emplace_back(v);
  }
}

static inline void get_value_u32(const uint8_t header,const uint8_t *&b_off, std::vector<uint32_t> &decoded) {
  uint32_t back = 0;
  ++b_off;

  switch (header) { // a length
    case 3:
      back |= (uint32_t)*b_off++ << 24;
      [[fallthrough]];
    case 2:
      back |= (uint32_t)*b_off++ << 16;
      [[fallthrough]];
    case 1:
      back |= (uint32_t)*b_off++ << 8;
      [[fallthrough]];
    case 0:
      if (back | *b_off)
        decoded.emplace_back(back | *b_off);
  }
}

static inline void get_value_u64(const uint8_t header, const uint8_t *&b_off, std::vector<uint64_t> &decoded) {
  uint64_t back = 0;
  ++b_off;

    switch (header) { // a length
      case 7:
        back |= (uint64_t)*b_off++ << 56;
        [[fallthrough]];
      case 6:
        back |= (uint64_t)*b_off++ << 48;
        [[fallthrough]];
      case 5:
        back |= (uint64_t)*b_off++ << 40;
        [[fallthrough]];
      case 4:
        back |= (uint64_t)*b_off++ << 32;
        [[fallthrough]];
      case 3:
        back |= (uint64_t)*b_off++ << 24;
        [[fallthrough]];
      case 2:
        back |= (uint64_t)*b_off++ << 16;
        [[fallthrough]];
      case 1:
        back |= (uint64_t)*b_off++ << 8;
        [[fallthrough]];
      case 0:
        if (back | *b_off)
          decoded.emplace_back(back | *b_off);
    }
}

const std::vector<uint8_t>
gvb_encode_uint32_t(const std::vector<uint32_t> &nums) {
  std::vector<uint8_t> encoded;
  encoded.reserve(((nums.size() + 3) / 4) * 17 + 3); // Maximum size of encoded

  size_t i;
  for (i = 0; i + 3 < nums.size(); i += 4) {
    // Process header
    const uint32_t a = nums[i];
    const uint32_t b = nums[i + 1];
    const uint32_t c = nums[i + 2];
    const uint32_t d = nums[i + 3];

    const uint8_t length_a = get_length(a);
    const uint8_t length_b = get_length(b);
    const uint8_t length_c = get_length(c);
    const uint8_t length_d = get_length(d);

    encoded.emplace_back(length_a << 6 | length_b << 4 | length_c << 2 |
                         length_d);

    // Process a
    put_value(encoded, a, length_a);

    // Process b
    put_value(encoded, b, length_b);

    // Process c
    put_value(encoded, c, length_c);

    // Process d
    put_value(encoded, d, length_d);
  }

  // If input is not divisible by 4, process last few ints and pad output so it
  // is divisible by 4
  if (uint8_t r = nums.size() % 4) {
    uint32_t a, b, c;
    uint8_t length_a, length_b, length_c;
    uint8_t header = 0;

    switch (r) {
    case 3:
      c = nums[i + 2];
      length_c = get_length(c);
      header |= length_c << 2;
      [[fallthrough]];
    case 2:
      b = nums[i + 1];
      length_b = get_length(b);
      header |= length_b << 4;
      [[fallthrough]];
    case 1:
      a = nums[i];
      length_a = get_length(a);
      header |= length_a << 6;
    }

    encoded.emplace_back(header);

    // Process a
    put_value(encoded, a, length_a);

    // Process b
    if (r > 1) {
      put_value(encoded, b, length_b);
    }

    // Process c
    if (r > 2) {
      put_value(encoded, c, length_c);
    }

    // Pad remaining values
    for (uint8_t k = r; k < 4; ++k)
      encoded.emplace_back(0);
  }


  return encoded;
}

void
gvb_encode_uint32_t(const std::vector<uint32_t> &nums, std::vector<uint8_t> &encoded) {
  size_t i;
  for (i = 0; i + 3 < nums.size(); i += 4) {
    // Process header
    const uint32_t a = nums[i];
    const uint32_t b = nums[i + 1];
    const uint32_t c = nums[i + 2];
    const uint32_t d = nums[i + 3];

    const uint8_t length_a = get_length(a);
    const uint8_t length_b = get_length(b);
    const uint8_t length_c = get_length(c);
    const uint8_t length_d = get_length(d);

    encoded.emplace_back(length_a << 6 | length_b << 4 | length_c << 2 |
                         length_d);

    // Process a
    put_value(encoded, a, length_a);

    // Process b
    put_value(encoded, b, length_b);

    // Process c
    put_value(encoded, c, length_c);

    // Process d
    put_value(encoded, d, length_d);
  }

  // If input is not divisible by 4, process last few ints and pad output so it
  // is divisible by 4
  if (uint8_t r = nums.size() % 4) {
    uint32_t a, b, c;
    uint8_t length_a, length_b, length_c;
    uint8_t header = 0;

    switch (r) {
    case 3:
      c = nums[i + 2];
      length_c = get_length(c);
      header |= length_c << 2;
      [[fallthrough]];
    case 2:
      b = nums[i + 1];
      length_b = get_length(b);
      header |= length_b << 4;
      [[fallthrough]];
    case 1:
      a = nums[i];
      length_a = get_length(a);
      header |= length_a << 6;
    }

    encoded.emplace_back(header);

    // Process a
    put_value(encoded, a, length_a);

    // Process b
    if (r > 1) {
      put_value(encoded, b, length_b);
    }

    // Process c
    if (r > 2) {
      put_value(encoded, c, length_c);
    }

    // Pad remaining values
    for (uint8_t k = r; k < 4; ++k)
      encoded.emplace_back(0);
  }
}

const std::vector<uint8_t>
gvb_encode_uint64_t(const std::vector<uint64_t> &nums) {
  std::vector<uint8_t> encoded;
  encoded.reserve((nums.size() / 4) * 17); // Maximum size of encoded

  size_t i;
  for (i = 0; i + 1 < nums.size(); i += 2) {
    // Process header
    const uint64_t a = nums[i];
    const uint64_t b = nums[i + 1];

    const uint8_t length_a = get_length(a);
    const uint8_t length_b = get_length(b);
    encoded.emplace_back(length_a << 4 | length_b);

    // Process a
    put_value(encoded, a, length_a);

    // Process b
    put_value(encoded, b, length_b);
  }

  // If input is not even, process last int and pad output by 1
  if (nums.size() % 2 != 0) {
    const uint64_t a = nums[i];
    const uint8_t length_a = get_length(a);
    encoded.emplace_back(length_a << 4);

    put_value(encoded, a, length_a);

    encoded.emplace_back(0);
  }

  return encoded;
}

const std::vector<uint32_t>
gvb_decode_uint32_t(const std::vector<uint8_t> &bytes) {
  std::vector<uint32_t> decoded;
  decoded.reserve(bytes.size() * 4); // Maximum size

  const uint8_t *b_off = bytes.data();

  while (b_off < bytes.data() + bytes.size()) {
    const uint8_t header = b_off[0];

    // Decode a
    get_value_u32(header >> 6, b_off, decoded);

    // Decode b
    get_value_u32((header & 48) >> 4, b_off, decoded);

    // Decode c
    get_value_u32((header & 12) >> 2, b_off, decoded);

    // Deocde d
    get_value_u32((header & 3), b_off, decoded);

    ++b_off;
  }

  return decoded;
}

void
gvb_decode_uint32_t(const std::vector<uint8_t> &bytes, std::vector<uint32_t> &decoded) {
  const uint8_t *b_off = bytes.data();

  while (b_off < bytes.data() + bytes.size()) {
    const uint8_t header = b_off[0];

    // Decode a
    get_value_u32(header >> 6, b_off, decoded);

    // Decode b
    get_value_u32((header & 48) >> 4, b_off, decoded);

    // Decode c
    get_value_u32((header & 12) >> 2, b_off, decoded);

    // Deocde d
    get_value_u32((header & 3), b_off, decoded);

    ++b_off;
  }
}

const std::vector<uint64_t>
gvb_decode_uint64_t(const std::vector<uint8_t> &bytes) {
  std::vector<uint64_t> decoded;
  decoded.reserve(bytes.size() * 4); // Maximum size

  const uint8_t *b_off = bytes.data();

  while (b_off < bytes.data() + bytes.size()) {
    const uint8_t header = b_off[0];

    // Decode a
    get_value_u64((header >> 4), b_off, decoded);

    // Decode b
    get_value_u64((header & 15), b_off, decoded);

    ++b_off;
  }

  return decoded;
}