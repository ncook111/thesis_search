#ifndef CACHE_FLUSH_H
#define CACHE_FLUSH_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>

// https://stackoverflow.com/questions/34460744/flushing-the-cache-to-prevent-benchmarking-fluctiations/49077734#49077734

void flush_cache(size_t cache_bytes);

#endif