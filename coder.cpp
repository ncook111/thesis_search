#include "coder.h"
#include <iostream>
#include <random>
#include <cstdint>
#include <sys/time.h>
#include <sys/resource.h>

constexpr uint8_t BYTE_FST = 0x80; // Byte with most significant bit set
constexpr uint8_t BYTE_RST = 0x7F; // Byte with last 7 bits set
constexpr uint32_t UINT24_MAX = 0xFFFFFF;
constexpr long UINT40_MAX = 0xFFFFFFFFFF;
constexpr long UINT48_MAX = 0xFFFFFFFFFFFF;
constexpr long UINT56_MAX = 0xFFFFFFFFFFFFFF;

const long get_memory_usage() {
  struct rusage usage;
  if(0 == getrusage(RUSAGE_SELF, &usage))
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

const std::vector<uint8_t> gvb_encode_uint32_t(const std::vector<uint32_t> &nums) {
    std::vector<uint8_t> encoded;
    encoded.reserve((nums.size() / 4) * 17); // Maximum size of encoded
    
    size_t i;
    for (i  = 0; i + 3 < nums.size(); i += 4) {
        // Process header
        const uint32_t a = nums[i];
        const uint32_t b = nums[i+1];
        const uint32_t c = nums[i+2];
        const uint32_t d = nums[i+3];

        const uint8_t length_a = get_length(a);
        const uint8_t length_b = get_length(b);
        const uint8_t length_c = get_length(c);
        const uint8_t length_d = get_length(d);

        encoded.emplace_back(length_a << 6 | length_b << 4 | length_c << 2 | length_d);

        // Process a
        switch(length_a) {
            case 3: encoded.push_back(a >> 24); [[fallthrough]];
            case 2: encoded.push_back(a >> 16); [[fallthrough]];
            case 1: encoded.push_back(a >> 8); [[fallthrough]];
            case 0: encoded.push_back(a);
        }

        // Process b 
        switch(length_b) {
            case 3: encoded.push_back(b >> 24); [[fallthrough]];
            case 2: encoded.push_back(b >> 16); [[fallthrough]];
            case 1: encoded.push_back(b >> 8); [[fallthrough]];
            case 0: encoded.push_back(b);
        }

        // Process c
        switch(length_c) {
            case 3: encoded.push_back(c >> 24); [[fallthrough]];
            case 2: encoded.push_back(c >> 16); [[fallthrough]];
            case 1: encoded.push_back(c >> 8); [[fallthrough]];
            case 0: encoded.push_back(c);
        }

        // Process d 
        switch(length_d) {
            case 3: encoded.push_back(d >> 24); [[fallthrough]];
            case 2: encoded.push_back(d >> 16); [[fallthrough]];
            case 1: encoded.push_back(d >> 8); [[fallthrough]];
            case 0: encoded.push_back(d);
        }
    }

    // If input is not divisible by 4, process last few ints and pad output so it is divisible by 4
    if (uint8_t r = nums.size() % 4) {
        uint32_t a, b, c, d;
        uint8_t length_a, length_b, length_c;
        uint8_t header;

        switch (r) {
            case 3:
                c = nums[i+2];
                length_c = get_length(c);
                header = length_c << 2;
            case 2:
                b = nums[i+1];
                length_b = get_length(b);
                header = length_b << 4;
            case 1:
                a = nums[i];
                length_c = get_length(a);
                header = length_c << 6;
        }

        encoded.emplace_back(header);

        // Process a
        switch(length_a) {
            case 3: encoded.emplace_back(a >> 24); [[fallthrough]];
            case 2: encoded.emplace_back(a >> 16); [[fallthrough]];
            case 1: encoded.emplace_back(a >> 8); [[fallthrough]];
            case 0: encoded.emplace_back(a);
        }

        // Process b
        if (r > 1) {
            switch(length_b) {
                case 3: encoded.emplace_back(b >> 24); [[fallthrough]];
                case 2: encoded.emplace_back(b >> 16); [[fallthrough]];
                case 1: encoded.emplace_back(b >> 8); [[fallthrough]];
                case 0: encoded.emplace_back(b);
            }
        }

        // Process c
        if (r > 2) {
            switch(length_c) {
                case 3: encoded.emplace_back(c >> 24); [[fallthrough]];
                case 2: encoded.emplace_back(c >> 16); [[fallthrough]];
                case 1: encoded.emplace_back(c >> 8); [[fallthrough]];
                case 0: encoded.emplace_back(c);
            }
        }
    }
    
    return encoded;
}

const std::vector<uint8_t> gvb_encode_uint64_t(const std::vector<uint64_t> &nums) {
    std::vector<uint8_t> encoded;
    encoded.reserve((nums.size() / 4) * 17); // Maximum size of encoded

    size_t i;
    for (i = 0; i + 1 < nums.size(); i += 2) {
        // Process header
        const uint64_t a = nums[i];
        const uint64_t b = nums[i+1];

        const uint8_t length_a = get_length(a);
        const uint8_t length_b = get_length(b);
        encoded.emplace_back(length_a << 4 | length_b);

        // Process a
        switch(length_a) {
            case 7: encoded.emplace_back(a >> 56); [[fallthrough]];
            case 6: encoded.emplace_back(a >> 48); [[fallthrough]];
            case 5: encoded.emplace_back(a >> 40); [[fallthrough]];
            case 4: encoded.emplace_back(a >> 32); [[fallthrough]];
            case 3: encoded.emplace_back(a >> 24); [[fallthrough]];
            case 2: encoded.emplace_back(a >> 16); [[fallthrough]];
            case 1: encoded.emplace_back(a >> 8); [[fallthrough]];;
            case 0: encoded.emplace_back(a);
        }

        // Process b 
        switch(length_b) {
            case 7: encoded.emplace_back(b >> 56); [[fallthrough]];
            case 6: encoded.emplace_back(b >> 48); [[fallthrough]];
            case 5: encoded.emplace_back(b >> 40); [[fallthrough]];
            case 4: encoded.emplace_back(b >> 32); [[fallthrough]];
            case 3: encoded.emplace_back(b >> 24); [[fallthrough]];
            case 2: encoded.emplace_back(b >> 16); [[fallthrough]];
            case 1: encoded.emplace_back(b >> 8); [[fallthrough]];
            case 0: encoded.emplace_back(b);
        }
    }

    // If input is not even, process last int and pad output by 1
    if (nums.size() % 2 != 0) {
        const uint64_t a = nums[i];
        const uint8_t length_a = get_length(a);
        encoded.emplace_back(length_a << 4);

        switch(length_a) {
            case 7: encoded.emplace_back(a >> 56); [[fallthrough]];
            case 6: encoded.emplace_back(a >> 48); [[fallthrough]];
            case 5: encoded.emplace_back(a >> 40); [[fallthrough]];
            case 4: encoded.emplace_back(a >> 32); [[fallthrough]];
            case 3: encoded.emplace_back(a >> 24); [[fallthrough]];
            case 2: encoded.emplace_back(a >> 16); [[fallthrough]];
            case 1: encoded.emplace_back(a >> 8); [[fallthrough]];
            case 0: encoded.emplace_back(a);
        }

        encoded.emplace_back(0);
    }
    
    return encoded;
}

const std::vector<uint32_t> gvb_decode_uint32_t(const std::vector<uint8_t> &bytes) {
    std::vector<uint32_t> decoded;
    decoded.reserve(bytes.size() * 4); // Maximum size

    uint32_t back;
    const uint8_t* b_off = bytes.data();

    while (b_off < bytes.data() + bytes.size()) {
        const uint8_t header = b_off[0];

        // Decode a
        ++b_off;
        back = 0;
        switch(header >> 6) { // a length
            case 3: back |= (uint32_t)*b_off++ << 24; [[fallthrough]];
            case 2: back |= (uint32_t)*b_off++ << 16; [[fallthrough]];
            case 1: back |= (uint32_t)*b_off++ << 8; [[fallthrough]];
            case 0: if (back | *b_off) decoded.emplace_back(back | *b_off);
        }

        // Decode b
        ++b_off;
        back = 0;

        switch((header & 48) >> 4) { // b length
            case 3: back |= (uint32_t)*b_off++ << 24; [[fallthrough]];
            case 2: back |= (uint32_t)*b_off++ << 16; [[fallthrough]];
            case 1: back |= (uint32_t)*b_off++ << 8; [[fallthrough]];
            case 0: if (back | *b_off) decoded.emplace_back(back | *b_off);
        }

        // Decode c
        ++b_off;
        back = 0;

        switch((header & 12) >> 2) { // c length
            case 3: back |= (uint32_t)*b_off++ << 24; [[fallthrough]];
            case 2: back |= (uint32_t)*b_off++ << 16; [[fallthrough]];
            case 1: back |= (uint32_t)*b_off++ << 8; [[fallthrough]];
            case 0: if (back | *b_off) decoded.emplace_back(back | *b_off);
        }

        // Deocde d
        ++b_off;
        back = 0;

        switch((header & 3)) { // d length
            case 3: back |= (uint32_t)*b_off++ << 24; [[fallthrough]];
            case 2: back |= (uint32_t)*b_off++ << 16; [[fallthrough]];
            case 1: back |= (uint32_t)*b_off++ << 8; [[fallthrough]];
            case 0: if (back | *b_off) decoded.emplace_back(back | *b_off);
        }

        ++b_off;
    }

    return decoded;
}

const std::vector<uint64_t> gvb_decode_uint64_t(const std::vector<uint8_t> &bytes) {
    std::vector<uint64_t> decoded;
    decoded.reserve(bytes.size() * 4); // Maximum size

    uint64_t back = 0;
    const uint8_t* b_off = bytes.data();

    while (b_off < bytes.data() + bytes.size()) {
        const uint8_t header = b_off[0];

        // Decode a
        ++b_off;
        back = 0;
        switch(header >> 4) { // a length
            case 7: back |= (uint64_t)*b_off++ << 56; [[fallthrough]];
            case 6: back |= (uint64_t)*b_off++ << 48; [[fallthrough]];
            case 5: back |= (uint64_t)*b_off++ << 40; [[fallthrough]];
            case 4: back |= (uint64_t)*b_off++ << 32; [[fallthrough]];
            case 3: back |= (uint64_t)*b_off++ << 24; [[fallthrough]];
            case 2: back |= (uint64_t)*b_off++ << 16; [[fallthrough]];
            case 1: back |= (uint64_t)*b_off++ << 8; [[fallthrough]];
            case 0: if (back | *b_off) decoded.emplace_back(back | *b_off);
        }

        // Decode b
        ++b_off;
        back = 0;
        switch((header & 15)) { // b length
            case 7: back |= (uint64_t)*b_off++ << 56; [[fallthrough]];
            case 6: back |= (uint64_t)*b_off++ << 48; [[fallthrough]];
            case 5: back |= (uint64_t)*b_off++ << 40; [[fallthrough]];
            case 4: back |= (uint64_t)*b_off++ << 32; [[fallthrough]];
            case 3: back |= (uint64_t)*b_off++ << 24; [[fallthrough]];
            case 2: back |= (uint64_t)*b_off++ << 16; [[fallthrough]];
            case 1: back |= (uint64_t)*b_off++ << 8; [[fallthrough]];
            case 0: if (back | *b_off) decoded.emplace_back(back | *b_off);
        }

        ++b_off;
    }

    return decoded;
}