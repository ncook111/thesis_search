#include "../src/coder.h"
#include "../src/pef.h"
#include "../src/fls.h"
#include "../src/simdbp128.h"
#include "../src/sse_neon_delta.h"
#include "../src/avx2_delta.h"
#include "../src/avx512_delta.h"
#include "deltatemplates.h"
#include "cache_flush.h"
#include <boost/align/aligned_allocator.hpp>
#include <benchmark/benchmark.h>
#include <random>
#include <set>
#include <map>
#if defined(__linux__)
#include <malloc.h>
#endif

using aligned_vec = std::vector<uint32_t, boost::alignment::aligned_allocator<uint32_t, 64>>;

constexpr size_t cache_size = 4096 * 1024;

#if defined(__linux__)
static void reset_rss_hwm() { std::ofstream("/proc/self/clear_refs") << "5"; }

static size_t read_rss_kb(const char* key) {
    std::ifstream f("/proc/self/status");
    for (std::string line; std::getline(f, line);)
        if (line.rfind(key, 0) == 0) return std::stoul(line.substr(std::strlen(key)));
    return 0;
}
#endif

static benchmark::Counter kb_counter(double kb) {
    return benchmark::Counter(kb * 1024.0, benchmark::Counter::kDefaults,
                              benchmark::Counter::kIs1024);
}

// Trims the heap and returns the resident set size (kB) to use as the baseline
// for read_mem_end. Returns 0 on platforms without /proc.
static double read_mem_begin() {
    #if defined(__linux__)
    malloc_trim(0);
    return read_rss_kb("VmRSS:");
    #else
    return 0.0;
    #endif
}

// Records peak_rss / peak_delta / residual counters relative to the baseline
// returned by read_mem_begin. No-op on platforms without /proc.
static void read_mem_end(benchmark::State& state, double rss_before) {
    #if defined(__linux__)
    const double peak_kb = read_rss_kb("VmHWM:");
    malloc_trim(0);
    const double rss_trimmed = read_rss_kb("VmRSS:");

    state.counters["peak_rss"]       = kb_counter(peak_kb);
    state.counters["peak_delta"]     = kb_counter(peak_kb - rss_before);
    state.counters["residual"]       = kb_counter(rss_trimmed - rss_before);
    #else
    (void)state;
    (void)rss_before;
    #endif
}

static double get_percentile(std::vector<double> timings, double percentile) {
  std::sort(timings.begin(), timings.end());
  return timings[static_cast<size_t>(percentile * (timings.size() - 1))];
}

static void StandardBenchArgs(benchmark::internal::Benchmark* args) {
    args->RangeMultiplier(10)
     ->Range(1, 100000000)   // 1, 10, 100, 1k, 10k, 100k, 1M, 10M
     ->Unit(benchmark::kMicrosecond)
     ->Iterations(1)
     ->UseRealTime()
     ->ComputeStatistics("p95", [](const std::vector<double>& v) { return get_percentile(v, 0.99); })
     ->ComputeStatistics("p5", [](const std::vector<double>& v) { return get_percentile(v, 0.01); })
     ->ComputeStatistics("max", [](const std::vector<double>& v) { return *std::max_element(v.begin(), v.end()); });
}

std::vector<uint32_t> generate_uint32_t_dist(size_t size) {
    std::mt19937 eng(42);
    std::uniform_int_distribution<uint32_t> distr_32;

    std::set<uint32_t> input_set;
    std::vector<uint32_t> input;

    const int per_range = size / 4;

    const uint32_t moduli[] = {
        0xFF,
        0xFFFF,
        0xFFFFFF,
        0xFFFFFFFF
    };

    for (uint32_t mod : moduli) {
        int added = 0;

        if (mod < per_range) {
            // Start at 1 to skip 0
            for (uint32_t i = 1; i < mod; ++i) {
                input_set.insert(i);
            }
        } else {
            while (added < per_range) {
                uint32_t val = distr_32(eng) % mod;
                if (val != 0 && input_set.insert(val).second) {
                    ++added;
                }
            }
        }
    }

    // Likely will be short some values, insert the rest
    size_t leftover = size - input_set.size();
    while (leftover > 0) {
        uint32_t val = distr_32(eng); // full 32-bit range, no modulo bias
        if (val != 0 && input_set.insert(val).second) {
            --leftover;
        }
    }

    return std::vector<uint32_t>(input_set.begin(), input_set.end());
}

const std::vector<uint32_t> &dataset(size_t n) {
    static std::map<size_t, std::vector<uint32_t>> cache;
    auto it = cache.find(n);
    if (it == cache.end())
        it = cache.emplace(n, generate_uint32_t_dist(n)).first;
    return it->second;
}

class U32Vec : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;

    void SetUp(::benchmark::State& state) {
        input = dataset(state.range(0));
    }


    void TearDown(::benchmark::State& state) {
    }
};

class U32VecAligned : public benchmark::Fixture {
public:
    aligned_vec input;

    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> tmp = dataset(state.range(0));
        input.assign(tmp.begin(), tmp.end());
    }


    void TearDown(::benchmark::State& state) {
    }
};

class U32GVBDecode : public benchmark::Fixture {
public:
    std::vector<uint8_t> input;
    void SetUp(::benchmark::State& state) {
        input = gvb_encode_uint32_t(dataset(state.range(0)));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32GVBDeltaDecode : public benchmark::Fixture {
public:
    std::vector<uint8_t> input;
    void SetUp(::benchmark::State& state) {
        input = gvb_encode_uint32_t(delta_encode(dataset(state.range(0))));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32DeltaDecode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    void SetUp(::benchmark::State& state) {
        input = delta_encode(dataset(state.range(0)));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32PEFDecode : public benchmark::Fixture {
public:
    succinct::bit_vector input;
    Pef_Header header;

    void SetUp(::benchmark::State& state) {
        header = partitioned_ef_encode<uint32_t>(input, dataset(state.range(0)));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32FLSDecode : public benchmark::Fixture {
public:
    std::vector<uint8_t> input;
    size_t num_consumed;
    size_t num_blocks;

    void SetUp(::benchmark::State& state) {
        input.clear();
        std::vector<uint8_t> enc_num_blocks = fls_encode_uint32_t(dataset(state.range(0)), input);
        num_blocks = vb_decode_num<uint64_t>(enc_num_blocks, num_consumed);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S1DTDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    aligned_vec base;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> src = dataset(state.range(0));
        std::vector<uint32_t> enc, enc_base;
        fls_delta_encode_scalar(src, enc, enc_base);
        input.assign(enc.begin(), enc.end());
        base.assign(enc_base.begin(), enc_base.end());
    }

    void TearDown(::benchmark::State& state) {
    }
};

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
class U32S4BPD1Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = dataset(state.range(0));;
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::S4_BPD1);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4BPD2Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = dataset(state.range(0));
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::S4_BPD2);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4BPD4Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = dataset(state.range(0));
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::S4_BPD4);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4BPDMDecode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = dataset(state.range(0));;
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::S4_BPDM);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4D1Decode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s4_delta_encode<RegularDeltaSIMD>(dataset(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4D2Decode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s4_delta_encode<CoarseDelta2SIMD>(dataset(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4D4Decode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s4_delta_encode<CoarseDelta4SIMD>(dataset(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4DMDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s4_delta_encode<Max4DeltaSIMD>(dataset(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

// fls_delta_decode_sse only exists under SIMD_TARGET_SSE, so this is not built for NEON.
#if defined(SIMD_TARGET_SSE)
class U32S4DTDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    aligned_vec base;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> src = dataset(state.range(0));
        std::vector<uint32_t> enc, enc_base;
        fls_delta_encode_scalar(src, enc, enc_base);
        input.assign(enc.begin(), enc.end());
        base.assign(enc_base.begin(), enc_base.end());
    }

    void TearDown(::benchmark::State& state) {
    }
};
#endif
#endif

#if defined(SIMD_TARGET_AVX2)
class U32S8BPD4Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = dataset(state.range(0));;
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::S8_BPD4);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S8BPDMDecode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = dataset(state.range(0));;
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::S8_BPDM);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S8D4Decode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> tmp = dataset(state.range(0));
        aligned_vec src(tmp.begin(), tmp.end());
        input.resize(src.size());

        s8_delta_encode<CoarseDelta4AVX2>(src.data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S8DMDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> tmp = dataset(state.range(0));
        aligned_vec src(tmp.begin(), tmp.end());
        input.resize(src.size());

        s8_delta_encode<Max4DeltaAVX2>(src.data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S8DTDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    aligned_vec base;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> src = dataset(state.range(0));
        std::vector<uint32_t> enc, enc_base;
        fls_delta_encode_scalar(src, enc, enc_base);
        input.assign(enc.begin(), enc.end());
        base.assign(enc_base.begin(), enc_base.end());
    }

    void TearDown(::benchmark::State& state) {
    }
};
#endif

#if defined(SIMD_TARGET_AVX512BW)
class U32S16BPD4Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = dataset(state.range(0));
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::S16_BPD4);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S16BPDMDecode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = dataset(state.range(0));
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::S16_BPDM);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S16D4Decode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> tmp = dataset(state.range(0));
        aligned_vec src(tmp.begin(), tmp.end());
        input.resize(src.size());

        s16_delta_encode<CoarseDelta4AVX512>(src.data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S16DMDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> tmp = dataset(state.range(0));
        aligned_vec src(tmp.begin(), tmp.end());
        input.resize(src.size());

        s16_delta_encode<Max4DeltaAVX512>(src.data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S16DTDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    aligned_vec base;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> src = dataset(state.range(0));
        std::vector<uint32_t> enc, enc_base;
        fls_delta_encode_scalar(src, enc, enc_base);
        input.assign(enc.begin(), enc.end());
        base.assign(enc_base.begin(), enc_base.end());
    }

    void TearDown(::benchmark::State& state) {
    }
};
#endif

BENCHMARK_DEFINE_F(U32Vec, cenc_gvb)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint8_t> encoded;
    encoded = gvb_encode_uint32_t(input);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        encoded.clear();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);  
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        gvb_encode_uint32_t(input, encoded);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size());

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_delta)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = delta_encode(input);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        encoded.clear();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        delta_encode(input, encoded);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_gvb_delta)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint8_t> encoded;
    encoded = gvb_encode_uint32_t(delta_encode(input));
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        encoded.clear();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        gvb_encode_uint32_t(delta_encode(input), encoded);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size());

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_pef)(benchmark::State& state) {
    {
        succinct::bit_vector encoded;
        Pef_Header header;
        header = partitioned_ef_encode<uint32_t>(encoded, input);
        benchmark::DoNotOptimize(encoded);
        benchmark::DoNotOptimize(header);
        benchmark::ClobberMemory();
    }

    const double rss_before = read_mem_begin();
    succinct::bit_vector encoded;
    Pef_Header header;
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        header = partitioned_ef_encode<uint32_t>(encoded, input);
        benchmark::DoNotOptimize(encoded);
        benchmark::DoNotOptimize(header);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * 4);
    const double encoded_bytes = static_cast<double>(encoded.data().size() * sizeof(uint64_t) + sizeof(header));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_fls)(benchmark::State& state) {
    {
        std::vector<uint8_t> encoded;
        std::vector<uint8_t> num_blocks;
        num_blocks = fls_encode_uint32_t(input, encoded);
        benchmark::DoNotOptimize(encoded);
        benchmark::DoNotOptimize(num_blocks);
        benchmark::ClobberMemory();
    }

    const double rss_before = read_mem_begin();
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> num_blocks;
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        encoded.clear();
        num_blocks.clear();
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        num_blocks = fls_encode_uint32_t(input, encoded);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size());

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_s1dt)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    std::vector<uint32_t> base;
    fls_delta_encode_scalar(input, encoded, base);
    benchmark::DoNotOptimize(encoded);
    benchmark::DoNotOptimize(base);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        fls_delta_encode_scalar(input, encoded, base);
        benchmark::DoNotOptimize(encoded);
        benchmark::DoNotOptimize(base);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(state.range(0) * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_DEFINE_F(U32VecAligned, denc_s4d1)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec encoded(state.range(0));
    s4_delta_encode<RegularDeltaSIMD>(input.data(), encoded.data(), input.size());
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s4_delta_encode<RegularDeltaSIMD>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32VecAligned, denc_s4d2)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec encoded(state.range(0));
    s4_delta_encode<CoarseDelta2SIMD>(input.data(), encoded.data(), input.size());
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s4_delta_encode<CoarseDelta2SIMD>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32VecAligned, denc_s4d4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec encoded(state.range(0));
    s4_delta_encode<CoarseDelta4SIMD>(input.data(), encoded.data(), input.size());
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s4_delta_encode<CoarseDelta4SIMD>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32VecAligned, denc_s4dm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec encoded(state.range(0));
    s4_delta_encode<Max4DeltaSIMD>(input.data(), encoded.data(), input.size());
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s4_delta_encode<Max4DeltaSIMD>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s4_bpd1)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = encode_simdbp128(input, EncType::S4_BPD1);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        encode_simdbp128(input, encoded, EncType::S4_BPD1);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s4_bpd2)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = encode_simdbp128(input, EncType::S4_BPD2);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        encode_simdbp128(input, encoded, EncType::S4_BPD2);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s4_bpd4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = encode_simdbp128(input, EncType::S4_BPD4);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        encode_simdbp128(input, encoded, EncType::S4_BPD4);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s4_bpdm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = encode_simdbp128(input, EncType::S4_BPDM);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        encode_simdbp128(input, encoded, EncType::S4_BPDM);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_DEFINE_F(U32VecAligned, denc_s8d4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec encoded(state.range(0));
    s8_delta_encode<CoarseDelta4AVX2>(input.data(), encoded.data(), input.size());
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s8_delta_encode<CoarseDelta4AVX2>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32VecAligned, denc_s8dm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec encoded(state.range(0));
    s8_delta_encode<Max4DeltaAVX2>(input.data(), encoded.data(), input.size());
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s8_delta_encode<Max4DeltaAVX2>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s8_bpd4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = encode_simdbp128(input, EncType::S8_BPD4);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        encode_simdbp128(input, encoded, EncType::S8_BPD4);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s8_bpdm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = encode_simdbp128(input, EncType::S8_BPDM);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        encode_simdbp128(input, encoded, EncType::S8_BPDM);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_DEFINE_F(U32VecAligned, denc_s16d4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec encoded(state.range(0));
    s16_delta_encode<CoarseDelta4AVX512>(input.data(), encoded.data(), input.size());
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s16_delta_encode<CoarseDelta4AVX512>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32VecAligned, denc_s16dm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec encoded(state.range(0));
    s16_delta_encode<Max4DeltaAVX512>(input.data(), encoded.data(), input.size());
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s16_delta_encode<Max4DeltaAVX512>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s16_bpd4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = encode_simdbp128(input, EncType::S16_BPD4);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        encode_simdbp128(input, encoded, EncType::S16_BPD4);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s16_bpdm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> encoded;
    encoded = encode_simdbp128(input, EncType::S16_BPDM);
    benchmark::DoNotOptimize(encoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        encode_simdbp128(input, encoded, EncType::S16_BPDM);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["input_bytes"]   = input_bytes;
    state.counters["encoded_bytes"] = encoded_bytes;
    state.counters["compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}
#endif

BENCHMARK_DEFINE_F(U32GVBDecode, cdec_gvb)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = gvb_decode_uint32_t(input);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();
    
    for (auto _ : state) {
        state.PauseTiming();
        decoded.clear();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        gvb_decode_uint32_t(input, decoded);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size());
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32DeltaDecode, ddec_delta)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = delta_decode(input);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        decoded.clear();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        delta_decode(input, decoded);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32GVBDeltaDecode, cdec_gvb_delta)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = delta_decode(gvb_decode_uint32_t(input));
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        decoded.clear();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        delta_decode(gvb_decode_uint32_t(input), decoded);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size());
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32PEFDecode, cdec_pef)(benchmark::State& state) {
    {
        std::vector<uint32_t> decoded;
        decoded = partitioned_ef_decode<uint32_t>(input, header.universe, header.n);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decoded = partitioned_ef_decode<uint32_t>(input, header.universe, header.n);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.data().size() * sizeof(uint64_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32FLSDecode, cdec_fls)(benchmark::State& state) {
    {
        std::vector<uint32_t> decoded;
        decoded = fls_decode_uint32_t(input, num_blocks);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }
    
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decoded = fls_decode_uint32_t(input, num_blocks);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size());
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S1DTDecode, ddec_s1dt)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(input.size());
    fls_delta_decode_scalar(base, input, decoded);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        fls_delta_decode_scalar(base, input, decoded);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_DEFINE_F(U32S4BPD1Decode, cdec_s4_bpd1)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = decode_simdbp128(input, EncType::S4_BPD1, orig_size);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decode_simdbp128(input, decoded, EncType::S4_BPD1, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4BPD2Decode, cdec_s4_bpd2)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = decode_simdbp128(input, EncType::S4_BPD2, orig_size);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decode_simdbp128(input, decoded, EncType::S4_BPD2, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4BPD4Decode, cdec_s4_bpd4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = decode_simdbp128(input, EncType::S4_BPD4, orig_size);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decode_simdbp128(input, decoded, EncType::S4_BPD4, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4BPDMDecode, cdec_s4_bpdm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = decode_simdbp128(input, EncType::S4_BPDM, orig_size);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decode_simdbp128(input, decoded, EncType::S4_BPDM, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4D1Decode, ddec_s4d1)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(state.range(0));
    s4_delta_decode<RegularDeltaSIMD>(input.data(), decoded.data(), input.size());
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s4_delta_decode<RegularDeltaSIMD>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4D2Decode, ddec_s4d2)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(state.range(0));
    s4_delta_decode<CoarseDelta2SIMD>(input.data(), decoded.data(), input.size());
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s4_delta_decode<CoarseDelta2SIMD>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4D4Decode, ddec_s4d4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(state.range(0));
    s4_delta_decode<CoarseDelta4SIMD>(input.data(), decoded.data(), input.size());
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s4_delta_decode<CoarseDelta4SIMD>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4DMDecode, ddec_s4dm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(state.range(0));
    s4_delta_decode<Max4DeltaSIMD>(input.data(), decoded.data(), input.size());
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s4_delta_decode<Max4DeltaSIMD>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

#if defined(SIMD_TARGET_SSE)
BENCHMARK_DEFINE_F(U32S4DTDecode, ddec_s4dt)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(input.size());
    fls_delta_decode_sse(base, input, decoded);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        fls_delta_decode_sse(base, input, decoded);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}
#endif
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_DEFINE_F(U32S8BPD4Decode, cdec_s8_bpd4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = decode_simdbp128(input, EncType::S8_BPD4, orig_size);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decode_simdbp128(input, decoded, EncType::S8_BPD4, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S8BPDMDecode, cdec_s8_bpdm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = decode_simdbp128(input, EncType::S8_BPDM, orig_size);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decode_simdbp128(input, decoded, EncType::S8_BPDM, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S8D4Decode, ddec_s8d4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(state.range(0));
    s8_delta_decode<CoarseDelta4AVX2>(input.data(), decoded.data(), input.size());
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s8_delta_decode<CoarseDelta4AVX2>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S8DMDecode, ddec_s8dm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(state.range(0));
    s8_delta_decode<Max4DeltaAVX2>(input.data(), decoded.data(), input.size());
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s8_delta_decode<Max4DeltaAVX2>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S8DTDecode, ddec_s8dt)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(input.size());
    fls_delta_decode_avx2(base, input, decoded);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        fls_delta_decode_avx2(base, input, decoded);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_DEFINE_F(U32S16BPD4Decode, cdec_s16_bpd4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = decode_simdbp128(input, EncType::S16_BPD4, orig_size);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decode_simdbp128(input, decoded, EncType::S16_BPD4, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S16BPDMDecode, cdec_s16_bpdm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    std::vector<uint32_t> decoded;
    decoded = decode_simdbp128(input, EncType::S16_BPDM, orig_size);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        decode_simdbp128(input, decoded, EncType::S16_BPDM, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["input_bytes"] = input_bytes;
    state.counters["bits_per_int"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S16D4Decode, ddec_s16d4)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(state.range(0));
    s16_delta_decode<CoarseDelta4AVX512>(input.data(), decoded.data(), input.size());
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s16_delta_decode<CoarseDelta4AVX512>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S16DMDecode, ddec_s16dm)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(state.range(0));
    s16_delta_decode<Max4DeltaAVX512>(input.data(), decoded.data(), input.size());
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        s16_delta_decode<Max4DeltaAVX512>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S16DTDecode, ddec_s16dt)(benchmark::State& state) {
    const double rss_before = read_mem_begin();
    aligned_vec decoded(input.size());
    fls_delta_decode_avx512(base, input, decoded);
    benchmark::DoNotOptimize(decoded);
    benchmark::ClobberMemory();

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        #if defined(__linux__)
        malloc_trim(0);
        reset_rss_hwm();
        #endif
        state.ResumeTiming();
        fls_delta_decode_avx512(base, input, decoded);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    read_mem_end(state, rss_before);

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}
#endif

BENCHMARK_REGISTER_F(U32Vec, cenc_gvb)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_gvb_delta)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_pef)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_fls)->Apply(StandardBenchArgs);

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32Vec, cenc_s4_bpd4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_s4_bpdm)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_REGISTER_F(U32Vec, cenc_s8_bpd4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_s8_bpdm)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_REGISTER_F(U32Vec, cenc_s16_bpd4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_s16_bpdm)->Apply(StandardBenchArgs);
#endif

BENCHMARK_REGISTER_F(U32GVBDecode, cdec_gvb)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32GVBDeltaDecode, cdec_gvb_delta)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32PEFDecode, cdec_pef)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32FLSDecode, cdec_fls)->Apply(StandardBenchArgs);

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32S4BPD4Decode, cdec_s4_bpd4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S4BPDMDecode, cdec_s4_bpdm)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_REGISTER_F(U32S8BPD4Decode, cdec_s8_bpd4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S8BPDMDecode, cdec_s8_bpdm)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_REGISTER_F(U32S16BPD4Decode, cdec_s16_bpd4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S16BPDMDecode, cdec_s16_bpdm)->Apply(StandardBenchArgs);
#endif

BENCHMARK_REGISTER_F(U32Vec, denc_delta)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, denc_s1dt)->Apply(StandardBenchArgs);

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32VecAligned, denc_s4d4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32VecAligned, denc_s4dm)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_REGISTER_F(U32VecAligned, denc_s8d4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32VecAligned, denc_s8dm)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_REGISTER_F(U32VecAligned, denc_s16d4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32VecAligned, denc_s16dm)->Apply(StandardBenchArgs);
#endif

BENCHMARK_REGISTER_F(U32DeltaDecode, ddec_delta)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S1DTDecode, ddec_s1dt)->Apply(StandardBenchArgs);

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32S4D4Decode, ddec_s4d4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S4DMDecode, ddec_s4dm)->Apply(StandardBenchArgs);
#if defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32S4DTDecode, ddec_s4dt)->Apply(StandardBenchArgs);
#endif
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_REGISTER_F(U32S8D4Decode, ddec_s8d4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S8DMDecode, ddec_s8dm)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S8DTDecode, ddec_s8dt)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_REGISTER_F(U32S16D4Decode, ddec_s16d4)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S16DMDecode, ddec_s16dm)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S16DTDecode, ddec_s16dt)->Apply(StandardBenchArgs);
#endif