#include "cache_flush.h"

void flush_cache(size_t cache_bytes) {
    // prevent compiler from optimising certain values away
    volatile void* ptr_sink = nullptr;
    volatile uint64_t sum_sink = 0;

    size_t size = cache_bytes * 2;
    size_t line_size = 64;
    size_t num_lines = size / line_size;

    size = num_lines * line_size;

    uint8_t* buf = static_cast<uint8_t*>(std::aligned_alloc(line_size, size));

    std::memset(buf, 1, size);

    ptr_sink = buf;

    constexpr int kTouchesPerLine = 10;
    std::vector<size_t> indices;
    indices.resize(num_lines * kTouchesPerLine);
    std::mt19937_64 eng(12345);
    std::uniform_int_distribution<size_t> dist(0, num_lines - 1);
    for (auto& idx : indices) idx = dist(eng);

    uint64_t sum = 0;
    for (size_t idx : indices) {
      sum += buf[idx * line_size];
    }

    sum_sink = sum;

    std::free(buf);
}