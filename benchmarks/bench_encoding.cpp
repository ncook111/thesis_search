
#include "../src/coder.h"
#include "../src/pef.h"
#include "../src/fls.h"
#include "../src/simdbp128.h"
#include "cache_flush.h"
#include <benchmark/benchmark.h>
#include <random>

constexpr size_t cache_size = 4096 * 1024;

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

class U64GVBDecode : public benchmark::Fixture {
public:
    std::vector<uint8_t> input;
    void SetUp(::benchmark::State& state) {
        input = gvb_encode_uint64_t(generate_int_dist(state.range(0)));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U64DeltaDecode : public benchmark::Fixture {
public:
    std::vector<uint64_t> input;
    void SetUp(::benchmark::State& state) {
        input = delta_encode(generate_int_dist(state.range(0)));
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U64PEFDecode : public benchmark::Fixture {
public:
    succinct::bit_vector input;
    Pef_Header header;

    void SetUp(::benchmark::State& state) {
        header = partitioned_ef_encode_u64(input, generate_int_dist(state.range(0)));
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
        num_blocks = vb_decode_num_uint64_t(enc_num_blocks, num_consumed);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32BPD1Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::BPD1);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32BPD2Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::BPD2);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32BPD4Decode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::BPD4);
    }

    void TearDown(::benchmark::State& state) {
    }
};

class U32BPDMDecode : public benchmark::Fixture {
public:
    std::vector<uint32_t> input;
    size_t orig_size;
    void SetUp(::benchmark::State& state) {
        std::vector<uint32_t> vec = generate_uint32_t_dist(state.range(0));
        orig_size = vec.size();
        input = encode_simdbp128(vec, EncType::BPDM);
    }

    void TearDown(::benchmark::State& state) {
    }
};

BENCHMARK_DEFINE_F(U64Vec, gvb_u64_t_encode)(benchmark::State& state) {
    std::vector<uint8_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = gvb_encode_uint64_t(input);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint64_t));
    const double encoded_bytes = static_cast<double>(encoded.size());

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression(%)"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64Vec, delta_u64_t_encode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> encoded = delta_encode(input);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint64_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64Vec, gvb_delta_u64_t_encode)(benchmark::State& state) {
    std::vector<uint8_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = gvb_encode_uint64_t(delta_encode(input));
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint64_t));
    const double encoded_bytes = static_cast<double>(encoded.size());

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression(%)"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64Vec, pef_u64_t_encode)(benchmark::State& state) {
    succinct::bit_vector encoded;
    Pef_Header header;

    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        header = partitioned_ef_encode_u64(encoded, input);
        benchmark::DoNotOptimize(encoded);
        benchmark::DoNotOptimize(header);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * 8);
    const double encoded_bytes = static_cast<double>(encoded.data().size() * sizeof(uint64_t) + sizeof(header));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression(%)"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64Vec, fls_u64_t_encode)(benchmark::State& state) {
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
    state.counters["Compression(%)"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, simdbp128_d1_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::BPD1);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression(%)"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, simdbp128_d2_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::BPD2);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression(%)"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, simdbp128_d4_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::BPD4);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression(%)"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32Vec, simdbp128_dm_encode)(benchmark::State& state) {
    std::vector<uint32_t> encoded;
    
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        encoded = encode_simdbp128(input, EncType::BPDM);
        benchmark::DoNotOptimize(encoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint32_t));
    const double encoded_bytes = static_cast<double>(encoded.size() * sizeof(uint32_t));

    state.counters["InputBytes"]   = input_bytes;
    state.counters["EncodedBytes"] = encoded_bytes;
    state.counters["Compression(%)"] = encoded_bytes / input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64GVBDecode, gvb_u64_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = gvb_decode_uint64_t(input);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size());

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64DeltaDecode, delta_u64_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = delta_decode(input);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size() * sizeof(uint64_t));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64GVBDecode, gvb_delta_u64_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = delta_decode(gvb_decode_uint64_t(input));
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size());

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64PEFDecode, pef_u64_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = partitioned_ef_decode_u64(input, header.universe, header.n);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.data().size() * sizeof(uint64_t) + sizeof(header));

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U64FLSDecode, fls_u64_t_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint64_t> decoded = fls_decode_uint64_t(input, num_blocks);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes = static_cast<double>(input.size());

    state.counters["InputBytes"] = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32BPD1Decode, simdbp128_d1_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::BPD1, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size());

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32BPD2Decode, simdbp128_d2_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::BPD2, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size());

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32BPD4Decode, simdbp128_d4_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::BPD4, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size());

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_DEFINE_F(U32BPDMDecode, simdbp128_dm_decode)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        std::vector<uint32_t> decoded = decode_simdbp128(input, EncType::BPDM, orig_size);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }

    const double input_bytes   = static_cast<double>(input.size());

    state.counters["InputBytes"]   = input_bytes;

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_bytes));
}

BENCHMARK_REGISTER_F(U64Vec, gvb_u64_t_encode)
    ->Arg(1000) // 1k
    ->Arg(10000) // 10k
    ->Arg(100000) // 100k
    ->Arg(1000000) // 1m
    ->Arg(10000000) // 10m
    ->Iterations(50);

BENCHMARK_REGISTER_F(U64Vec, gvb_delta_u64_t_encode)
    ->Arg(1000) // 1k
    ->Arg(10000) // 10k
    ->Arg(100000) // 100k
    ->Arg(1000000) // 1m
    ->Arg(10000000) // 10m
    ->Iterations(50);

BENCHMARK_REGISTER_F(U64Vec, pef_u64_t_encode)
    ->Arg(1000) // 1k
    ->Arg(10000) // 10k
    ->Arg(100000) // 100k
    ->Arg(1000000) // 1m
    ->Arg(10000000) // 10m
    ->Iterations(50);

BENCHMARK_REGISTER_F(U64Vec, fls_u64_t_encode)
    ->Arg(1000) // 1k
    ->Arg(10000) // 10k
    ->Arg(100000) // 100k
    ->Arg(1000000) // 1m
    ->Arg(10000000) // 10m
    ->Iterations(50);

BENCHMARK_REGISTER_F(U32Vec, simdbp128_d1_encode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U32Vec, simdbp128_d2_encode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U32Vec, simdbp128_d4_encode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U32Vec, simdbp128_dm_encode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U64GVBDecode, gvb_u64_t_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U64GVBDecode, gvb_delta_u64_t_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U64PEFDecode, pef_u64_t_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U64FLSDecode, fls_u64_t_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U32BPD1Decode, simdbp128_d1_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U32BPD2Decode, simdbp128_d2_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U32BPD4Decode, simdbp128_d4_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U32BPDMDecode, simdbp128_dm_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);

BENCHMARK_REGISTER_F(U64DeltaDecode, delta_u64_t_decode)
->Arg(1000) // 1k
->Arg(10000) // 10k
->Arg(100000) // 100k
->Arg(1000000) // 1m
->Arg(10000000) // 10m
->Iterations(50);