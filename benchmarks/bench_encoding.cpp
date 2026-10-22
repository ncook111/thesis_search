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

#define NUM_REPETITIONS 10

using aligned_vec = std::vector<uint32_t, boost::alignment::aligned_allocator<uint32_t, 64>>;

constexpr size_t cache_size = 4096 * 1024;

static double get_percentile(std::vector<double> timings, double percentile) {
  std::sort(timings.begin(), timings.end());
  return timings[static_cast<size_t>(percentile * (timings.size() - 1))];
}

static void StandardBenchArgs(benchmark::internal::Benchmark* args) {
    args->RangeMultiplier(10)
     ->Range(1000, 1000000)   // 1k, 10k, 100k, 1M
     ->Unit(benchmark::kMicrosecond)
     ->Iterations(1)
     ->Repetitions(NUM_REPETITIONS)
     ->ComputeStatistics("p90", [](const std::vector<double>& v) { return get_percentile(v, 0.90); })
     ->ComputeStatistics("p99", [](const std::vector<double>& v) { return get_percentile(v, 0.99); })
     ->ComputeStatistics("max", [](const std::vector<double>& v) { return *std::max_element(v.begin(), v.end()); });
}

std::vector<uint64_t> generate_int_dist(size_t size) {
    std::mt19937_64 eng(42);
    std::uniform_int_distribution<uint64_t> distr_64;

    std::set<uint64_t> input_set;
    std::vector<uint64_t> input;

    const int per_range = size / 8;

    const uint64_t moduli[] = {
        0xFF,
        0xFFFF,
        0xFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFFFF,
        0xFFFFFFFFFFFF,
        0xFFFFFFFFFFFFFF,
        0xFFFFFFFFFFFFFFFF
    };

    for (uint64_t mod : moduli) {
        int added = 0;

        if (mod < per_range) {
            // Start at 1 to skip 0
            for (uint64_t i = 1; i < mod; ++i) {
                input_set.insert(i);
            }
        } else {
            while (added < per_range) {
                uint64_t val = distr_64(eng) % mod;
                if (val != 0 && input_set.insert(val).second) {
                    ++added;
                }
            }
        }
    }

    // Likely will be short some values, insert the rest
    size_t leftover = size - input_set.size();
    while (leftover > 0) {
        uint64_t val = distr_64(eng); // full 64-bit range, no modulo bias
        if (val != 0 && input_set.insert(val).second) {
            --leftover;
        }
    }

    return std::vector<uint64_t>(input_set.begin(), input_set.end());
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

class U32Vec : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;

    void SetUp(::benchmark::State& state) {
        input = generate_uint32_t_dist(state.range(0));
    }


    void TearDown(::benchmark::State& state) {
    }
};

class U64Vec : public benchmark::Fixture {
public:
    std::vector<uint64_t> input;

    void SetUp(::benchmark::State& state) {
        input = generate_int_dist(state.range(0));
    }


    void TearDown(::benchmark::State& state) {
    }
};

class U32GVBDecode : public benchmark::Fixture {
public:
    std::vector<uint8_t> input;
    void SetUp(::benchmark::State& state) {
        input = gvb_encode_uint32_t(generate_uint32_t_dist(state.range(0)));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32GVBDeltaDecode : public benchmark::Fixture {
public:
    std::vector<uint8_t> input;
    void SetUp(::benchmark::State& state) {
        input = gvb_encode_uint32_t(delta_encode(generate_uint32_t_dist(state.range(0))));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32DeltaDecode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    void SetUp(::benchmark::State& state) {
        input = delta_encode(generate_uint32_t_dist(state.range(0)));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32PEFDecode : public benchmark::Fixture {
public:
    succinct::bit_vector input;
    Pef_Header header;

    void SetUp(::benchmark::State& state) {
        header = partitioned_ef_encode<uint32_t>(input, generate_uint32_t_dist(state.range(0)));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U64FLSDecode : public benchmark::Fixture {
public:
    std::vector<uint8_t> input;
    size_t num_consumed;
    size_t num_blocks;

    void SetUp(::benchmark::State& state) {
        input.clear();
        std::vector<uint8_t> enc_num_blocks = fls_encode_uint64_t(generate_int_dist(state.range(0)), input);
        num_blocks = vb_decode_num<uint64_t>(enc_num_blocks, num_consumed);
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
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
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
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
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
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
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
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
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
        s4_delta_encode<RegularDeltaSIMD>(generate_uint32_t_dist(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4D2Decode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s4_delta_encode<CoarseDelta2SIMD>(generate_uint32_t_dist(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4D4Decode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s4_delta_encode<CoarseDelta4SIMD>(generate_uint32_t_dist(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S4DMDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s4_delta_encode<Max4DeltaSIMD>(generate_uint32_t_dist(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};
#endif

#if defined(SIMD_TARGET_AVX2)
class U32S8BPD4Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
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
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
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
        input.resize(state.range(0));
        s8_delta_encode<CoarseDelta4AVX2>(generate_uint32_t_dist(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S8DMDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s8_delta_encode<Max4DeltaAVX2>(generate_uint32_t_dist(state.range(0)).data(), input.data(), input.size());
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
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
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
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
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
        input.resize(state.range(0));
        s16_delta_encode<CoarseDelta4AVX512>(generate_uint32_t_dist(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32S16DMDecode : public benchmark::Fixture {
public:
    aligned_vec input;
    void SetUp(::benchmark::State& state) {
        input.resize(state.range(0));
        s16_delta_encode<Max4DeltaAVX512>(generate_uint32_t_dist(state.range(0)).data(), input.data(), input.size());
    }

    void TearDown(::benchmark::State& state) {
    }
};
#endif

BENCHMARK_DEFINE_F(U32Vec, cenc_gvb_u32_t_encode)(benchmark::State& state) {
    std::vector<uint8_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = gvb_encode_uint32_t(input);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size());

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_delta_u32_t_encode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> encoded = delta_encode(input);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_gvb_delta_u32_t_encode)(benchmark::State& state) {
    std::vector<uint8_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = gvb_encode_uint32_t(delta_encode(input));
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size());

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_pef_u32_t_encode)(benchmark::State& state) {
    succinct::bit_vector encoded;
    Pef_Header header;

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        header = partitioned_ef_encode<uint32_t>(encoded, input);
        benchmark::DoNotOptimize(encoded);
        benchmark::DoNotOptimize(header);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * 4);
    const double encoded_bytes = static_cast<double>(encoded.data().size() * sizeof(uint32_t) + sizeof(header));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64Vec, cenc_fls_u64_t_encode)(benchmark::State& state) {
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> num_blocks;

    for (auto _ : state) {
        state.PauseTiming();
        encoded.clear();
        num_blocks.clear();
        flush_cache(cache_size);
        state.ResumeTiming();
        num_blocks = fls_encode_uint64_t(input, encoded);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * 8);
    const double encoded_bytes = static_cast<double>(encoded.size());

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_DEFINE_F(U32Vec, denc_s4d1_encode)(benchmark::State& state) {
    aligned_vec encoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s4_delta_encode<RegularDeltaSIMD>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_s4d2_encode)(benchmark::State& state) {
    aligned_vec encoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s4_delta_encode<CoarseDelta2SIMD>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_s4d4_encode)(benchmark::State& state) {
    aligned_vec encoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s4_delta_encode<CoarseDelta4SIMD>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_s4dm_encode)(benchmark::State& state) {
    aligned_vec encoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s4_delta_encode<Max4DeltaSIMD>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s4_bpd1_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::S4_BPD1);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s4_bpd2_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::S4_BPD2);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s4_bpd4_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::S4_BPD4);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s4_bpdm_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::S4_BPDM);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_DEFINE_F(U32Vec, denc_s8d4_encode)(benchmark::State& state) {
    aligned_vec encoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s8_delta_encode<CoarseDelta4AVX2>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_s8dm_encode)(benchmark::State& state) {
    aligned_vec encoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s8_delta_encode<Max4DeltaAVX2>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s8_bpd4_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::S8_BPD4);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s8_bpdm_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::S8_BPDM);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_DEFINE_F(U32Vec, denc_s16d4_encode)(benchmark::State& state) {
    aligned_vec encoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s16_delta_encode<CoarseDelta4AVX512>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_s16dm_encode)(benchmark::State& state) {
    aligned_vec encoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s16_delta_encode<Max4DeltaAVX512>(input.data(), encoded.data(), input.size());
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, cenc_s16_bpd4_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::S16_BPD4);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, denc_s16_bpdm_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::S16_BPDM);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}
#endif

BENCHMARK_DEFINE_F(U32GVBDecode, cdec_gvb_u32_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = gvb_decode_uint64_t(input);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size());
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32DeltaDecode, ddec_delta_u32_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = delta_decode(input);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32GVBDeltaDecode, cdec_gvb_delta_u32_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = delta_decode(gvb_decode_uint64_t(input));
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size());
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32PEFDecode, cdec_pef_u32_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = partitioned_ef_decode<uint64_t>(input, header.universe, header.n);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.data().size() * sizeof(uint64_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U64FLSDecode, cdec_fls_u64_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = fls_decode_uint64_t(input, num_blocks);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size());
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint64_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 8.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_DEFINE_F(U32S4BPD1Decode, cdec_s4_bpd1_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::S4_BPD1, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4BPD2Decode, cdec_s4_bpd2_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::S4_BPD2, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4BPD4Decode, cdec_s4_bpd4_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::S4_BPD4, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4BPDMDecode, cdec_s4_bpdm_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::S4_BPDM, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4D1Decode, ddec_s4d1_decode)(benchmark::State& state) {
    aligned_vec decoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s4_delta_decode<RegularDeltaSIMD>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4D2Decode, ddec_s4d2_decode)(benchmark::State& state) {
    aligned_vec decoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s4_delta_decode<CoarseDelta2SIMD>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4D4Decode, ddec_s4d4_decode)(benchmark::State& state) {
    aligned_vec decoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s4_delta_decode<CoarseDelta4SIMD>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S4DMDecode, ddec_s4dm_decode)(benchmark::State& state) {
    aligned_vec decoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s4_delta_decode<Max4DeltaSIMD>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_DEFINE_F(U32S8BPD4Decode, cdec_s8_bpd4_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::S8_BPD4, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S8BPDMDecode, cdec_s8_bpdm_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::S8_BPDM, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S8D4Decode, ddec_s8d4_decode)(benchmark::State& state) {
    aligned_vec decoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s8_delta_decode<CoarseDelta4AVX2>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S8DMDecode, ddec_s8dm_decode)(benchmark::State& state) {
    aligned_vec decoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s8_delta_decode<Max4DeltaAVX2>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_DEFINE_F(U32S16BPD4Decode, cdec_s16_bpd4_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::S16_BPD4, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S16BPDMDecode, cdec_s16_bpdm_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::S16_BPDM, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size() * sizeof(uint32_t));
    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.counters["InputBytes"] = input_bytes;
    state.counters["BitsPerInt"] = 4.0 * input_bytes / state.range(0);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S16D4Decode, ddec_s16d4_decode)(benchmark::State& state) {
    aligned_vec decoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s16_delta_decode<CoarseDelta4AVX512>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}

BENCHMARK_DEFINE_F(U32S16DMDecode, ddec_s16dm_decode)(benchmark::State& state) {
    aligned_vec decoded(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        s16_delta_decode<Max4DeltaAVX512>(input.data(), decoded.data(), input.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const int64_t output_bytes = int64_t(state.range(0)) * sizeof(uint32_t);

    state.SetBytesProcessed(int64_t(state.iterations()) * output_bytes);
}
#endif

BENCHMARK_REGISTER_F(U32Vec, cenc_gvb_u32_t_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_gvb_delta_u32_t_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_pef_u32_t_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U64Vec, cenc_fls_u64_t_encode)->Apply(StandardBenchArgs);

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32Vec, cenc_s4_bpd1_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_s4_bpd2_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_s4_bpd4_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_s4_bpdm_encode)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_REGISTER_F(U32Vec, cenc_s8_bpd4_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_s8_bpdm_encode)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_REGISTER_F(U32Vec, cenc_s16_bpd4_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, cenc_s16_bpdm_encode)->Apply(StandardBenchArgs);
#endif

BENCHMARK_REGISTER_F(U32GVBDecode, cdec_gvb_u32_t_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32GVBDeltaDecode, cdec_gvb_delta_u32_t_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32PEFDecode, cdec_pef_u32_t_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U64FLSDecode, cdec_fls_u64_t_decode)->Apply(StandardBenchArgs);

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32S4BPD1Decode, cdec_s4_bpd1_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S4BPD2Decode, cdec_s4_bpd2_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S4BPD4Decode, cdec_s4_bpd4_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S4BPDMDecode, cdec_s4_bpdm_decode)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_REGISTER_F(U32S8BPD4Decode, cdec_s8_bpd4_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S8BPDMDecode, cdec_s8_bpdm_decode)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_REGISTER_F(U32S16BPD4Decode, cdec_s16_bpd4_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S16BPDMDecode, cdec_s16_bpdm_decode)->Apply(StandardBenchArgs);
#endif

BENCHMARK_REGISTER_F(U32Vec, denc_delta_u32_t_encode)->Apply(StandardBenchArgs);

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32Vec, denc_s4d1_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, denc_s4d2_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, denc_s4d4_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, denc_s4dm_encode)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_REGISTER_F(U32Vec, denc_s8d4_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, denc_s8dm_encode)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_REGISTER_F(U32Vec, denc_s16d4_encode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32Vec, denc_s16dm_encode)->Apply(StandardBenchArgs);
#endif

BENCHMARK_REGISTER_F(U32DeltaDecode, ddec_delta_u32_t_decode)->Apply(StandardBenchArgs);

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
BENCHMARK_REGISTER_F(U32S4D1Decode, ddec_s4d1_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S4D2Decode, ddec_s4d2_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S4D4Decode, ddec_s4d4_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S4DMDecode, ddec_s4dm_decode)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX2)
BENCHMARK_REGISTER_F(U32S8D4Decode, ddec_s8d4_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S8DMDecode, ddec_s8dm_decode)->Apply(StandardBenchArgs);
#endif

#if defined(SIMD_TARGET_AVX512BW)
BENCHMARK_REGISTER_F(U32S16D4Decode, ddec_s16d4_decode)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(U32S16DMDecode, ddec_s16dm_decode)->Apply(StandardBenchArgs);
#endif