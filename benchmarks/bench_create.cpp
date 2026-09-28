#include <benchmark/benchmark.h>
#include <filesystem>
#include "cache_flush.h"
#include "../src/ingest.h"
#include "../src/helper.h"

constexpr size_t cache_size = 4096 * 1024;

static std::string csv_data_path = "../benchmarks/bench_data/all_input_data/csv/";
static std::string json_data_path = "../benchmarks/bench_data/all_input_data/json/";
static std::string index_path = "../benchmarks/bench_data/temp_indexes/index_1/";

static const std::vector<std::string> datasets = {"small", "medium", "large"};

class IndexCreate : public benchmark::Fixture {
public:
    std::string bench_path;
    std::string data_path;
    uint8_t enc_type;
    void SetUp(::benchmark::State& state) {
        int dataset_size = state.range(0);
        int dataset_number = state.range(1);
        uint8_t file_type = state.range(2);
        enc_type = state.range(3);
        std::filesystem::remove_all(index_path);
        std::string dataset = datasets[dataset_size];
        state.SetLabel(std::to_string(dataset_number+1) + "_" + dataset + "/" + enc_type_to_str(enc_type));
        bench_path = dataset + "_data/data_" + std::to_string(dataset_number+1);
        (file_type == FileType::CSV) ? data_path = csv_data_path : data_path = json_data_path;
    }
};

uint64_t get_folder_size(std::string path) {
    uint64_t total_size = 0;

    const std::filesystem::path folder_path(path);

    if (std::filesystem::exists(folder_path) && std::filesystem::is_directory(folder_path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder_path)) {
            if (entry.is_regular_file() && !entry.is_symlink()) {
                total_size += entry.file_size();
            }
        }
    }

    return total_size;
}

BENCHMARK_DEFINE_F(IndexCreate, index_create)(benchmark::State& state) {
    for (auto _ : state) {
        create_mode(index_path, data_path + bench_path, {}, enc_type);
        state.PauseTiming();
        const uint64_t data_size = get_folder_size(data_path + bench_path);
        const uint64_t index_size = get_folder_size(index_path);
        state.counters["IndexSize"]   = index_size;
        state.counters["DataSize"]   = data_size;
        state.counters["Compression(%)"] = (float)index_size / (float)data_size;
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}

/*
    Arg1: dataset size
    Arg2: dataset number
    Arg3: file type
    Arg4: encoding
*/
BENCHMARK_REGISTER_F(IndexCreate, index_create)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 1, FileType::CSV, EncType::GVB})
    ->Args({0, 2, FileType::CSV, EncType::GVB})
    ->Args({0, 3, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({0, 1, FileType::CSV, EncType::PEF})
    ->Args({0, 2, FileType::CSV, EncType::PEF})
    ->Args({0, 3, FileType::CSV, EncType::PEF})
    ->Args({0, 0, FileType::CSV, EncType::FLS})
    ->Args({0, 1, FileType::CSV, EncType::FLS})
    ->Args({0, 2, FileType::CSV, EncType::FLS})
    ->Args({0, 3, FileType::CSV, EncType::FLS})
#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
    ->Args({0, 0, FileType::CSV, EncType::S4_BPD1})
    ->Args({0, 1, FileType::CSV, EncType::S4_BPD1})
    ->Args({0, 2, FileType::CSV, EncType::S4_BPD1})
    ->Args({0, 3, FileType::CSV, EncType::S4_BPD1})
    ->Args({0, 0, FileType::CSV, EncType::S4_BPD2})
    ->Args({0, 1, FileType::CSV, EncType::S4_BPD2})
    ->Args({0, 2, FileType::CSV, EncType::S4_BPD2})
    ->Args({0, 3, FileType::CSV, EncType::S4_BPD2})
    ->Args({0, 0, FileType::CSV, EncType::S4_BPD4})
    ->Args({0, 1, FileType::CSV, EncType::S4_BPD4})
    ->Args({0, 2, FileType::CSV, EncType::S4_BPD4})
    ->Args({0, 3, FileType::CSV, EncType::S4_BPD4})
    ->Args({0, 0, FileType::CSV, EncType::S4_BPDM})
    ->Args({0, 1, FileType::CSV, EncType::S4_BPDM})
    ->Args({0, 2, FileType::CSV, EncType::S4_BPDM})
    ->Args({0, 3, FileType::CSV, EncType::S4_BPDM})
#endif
#if defined(SIMD_TARGET_AVX2)
    ->Args({0, 0, FileType::CSV, EncType::S8_BPD4})
    ->Args({0, 1, FileType::CSV, EncType::S8_BPD4})
    ->Args({0, 2, FileType::CSV, EncType::S8_BPD4})
    ->Args({0, 3, FileType::CSV, EncType::S8_BPD4})
    ->Args({0, 0, FileType::CSV, EncType::S8_BPDM})
    ->Args({0, 1, FileType::CSV, EncType::S8_BPDM})
    ->Args({0, 2, FileType::CSV, EncType::S8_BPDM})
    ->Args({0, 3, FileType::CSV, EncType::S8_BPDM})
#endif
#if defined(SIMD_TARGET_AVX512BW)
    ->Args({0, 0, FileType::CSV, EncType::S16_BPD4})
    ->Args({0, 1, FileType::CSV, EncType::S16_BPD4})
    ->Args({0, 2, FileType::CSV, EncType::S16_BPD4})
    ->Args({0, 3, FileType::CSV, EncType::S16_BPD4})
    ->Args({0, 0, FileType::CSV, EncType::S16_BPDM})
    ->Args({0, 1, FileType::CSV, EncType::S16_BPDM})
    ->Args({0, 2, FileType::CSV, EncType::S16_BPDM})
    ->Args({0, 3, FileType::CSV, EncType::S16_BPDM})
#endif
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, index_create)
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 1, FileType::CSV, EncType::GVB})
    ->Args({1, 2, FileType::CSV, EncType::GVB})
    ->Args({1, 3, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 1, FileType::CSV, EncType::PEF})
    ->Args({1, 2, FileType::CSV, EncType::PEF})
    ->Args({1, 3, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::FLS})
    ->Args({1, 1, FileType::CSV, EncType::FLS})
    ->Args({1, 2, FileType::CSV, EncType::FLS})
    ->Args({1, 3, FileType::CSV, EncType::FLS})
#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
    ->Args({1, 0, FileType::CSV, EncType::S4_BPD1})
    ->Args({1, 1, FileType::CSV, EncType::S4_BPD1})
    ->Args({1, 2, FileType::CSV, EncType::S4_BPD1})
    ->Args({1, 3, FileType::CSV, EncType::S4_BPD1})
    ->Args({1, 0, FileType::CSV, EncType::S4_BPD2})
    ->Args({1, 1, FileType::CSV, EncType::S4_BPD2})
    ->Args({1, 2, FileType::CSV, EncType::S4_BPD2})
    ->Args({1, 3, FileType::CSV, EncType::S4_BPD2})
    ->Args({1, 0, FileType::CSV, EncType::S4_BPD4})
    ->Args({1, 1, FileType::CSV, EncType::S4_BPD4})
    ->Args({1, 2, FileType::CSV, EncType::S4_BPD4})
    ->Args({1, 3, FileType::CSV, EncType::S4_BPD4})
    ->Args({1, 0, FileType::CSV, EncType::S4_BPDM})
    ->Args({1, 1, FileType::CSV, EncType::S4_BPDM})
    ->Args({1, 2, FileType::CSV, EncType::S4_BPDM})
    ->Args({1, 3, FileType::CSV, EncType::S4_BPDM})
#endif
#if defined(SIMD_TARGET_AVX2)
    ->Args({1, 0, FileType::CSV, EncType::S8_BPD4})
    ->Args({1, 1, FileType::CSV, EncType::S8_BPD4})
    ->Args({1, 2, FileType::CSV, EncType::S8_BPD4})
    ->Args({1, 3, FileType::CSV, EncType::S8_BPD4})
    ->Args({1, 0, FileType::CSV, EncType::S8_BPDM})
    ->Args({1, 1, FileType::CSV, EncType::S8_BPDM})
    ->Args({1, 2, FileType::CSV, EncType::S8_BPDM})
    ->Args({1, 3, FileType::CSV, EncType::S8_BPDM})
#endif
#if defined(SIMD_TARGET_AVX512BW)
    ->Args({1, 0, FileType::CSV, EncType::S16_BPD4})
    ->Args({1, 1, FileType::CSV, EncType::S16_BPD4})
    ->Args({1, 2, FileType::CSV, EncType::S16_BPD4})
    ->Args({1, 3, FileType::CSV, EncType::S16_BPD4})
    ->Args({1, 0, FileType::CSV, EncType::S16_BPDM})
    ->Args({1, 1, FileType::CSV, EncType::S16_BPDM})
    ->Args({1, 2, FileType::CSV, EncType::S16_BPDM})
    ->Args({1, 3, FileType::CSV, EncType::S16_BPDM})
#endif
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, index_create)
    ->Args({2, 0, FileType::CSV, EncType::GVB})
    ->Args({2, 1, FileType::CSV, EncType::GVB})
    ->Args({2, 2, FileType::CSV, EncType::GVB})
    ->Args({2, 3, FileType::CSV, EncType::GVB})
    ->Args({2, 0, FileType::CSV, EncType::PEF})
    ->Args({2, 1, FileType::CSV, EncType::PEF})
    ->Args({2, 2, FileType::CSV, EncType::PEF})
    ->Args({2, 3, FileType::CSV, EncType::PEF})
    ->Args({2, 0, FileType::CSV, EncType::FLS})
    ->Args({2, 1, FileType::CSV, EncType::FLS})
    ->Args({2, 2, FileType::CSV, EncType::FLS})
    ->Args({2, 3, FileType::CSV, EncType::FLS})
#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
    ->Args({2, 0, FileType::CSV, EncType::S4_BPD1})
    ->Args({2, 1, FileType::CSV, EncType::S4_BPD1})
    ->Args({2, 2, FileType::CSV, EncType::S4_BPD1})
    ->Args({2, 3, FileType::CSV, EncType::S4_BPD1})
    ->Args({2, 0, FileType::CSV, EncType::S4_BPD2})
    ->Args({2, 1, FileType::CSV, EncType::S4_BPD2})
    ->Args({2, 2, FileType::CSV, EncType::S4_BPD2})
    ->Args({2, 3, FileType::CSV, EncType::S4_BPD2})
    ->Args({2, 0, FileType::CSV, EncType::S4_BPD4})
    ->Args({2, 1, FileType::CSV, EncType::S4_BPD4})
    ->Args({2, 2, FileType::CSV, EncType::S4_BPD4})
    ->Args({2, 3, FileType::CSV, EncType::S4_BPD4})
    ->Args({2, 0, FileType::CSV, EncType::S4_BPDM})
    ->Args({2, 1, FileType::CSV, EncType::S4_BPDM})
    ->Args({2, 2, FileType::CSV, EncType::S4_BPDM})
    ->Args({2, 3, FileType::CSV, EncType::S4_BPDM})
#endif
#if defined(SIMD_TARGET_AVX2)
    ->Args({2, 0, FileType::CSV, EncType::S8_BPD4})
    ->Args({2, 1, FileType::CSV, EncType::S8_BPD4})
    ->Args({2, 2, FileType::CSV, EncType::S8_BPD4})
    ->Args({2, 3, FileType::CSV, EncType::S8_BPD4})
    ->Args({2, 0, FileType::CSV, EncType::S8_BPDM})
    ->Args({2, 1, FileType::CSV, EncType::S8_BPDM})
    ->Args({2, 2, FileType::CSV, EncType::S8_BPDM})
    ->Args({2, 3, FileType::CSV, EncType::S8_BPDM})
#endif
#if defined(SIMD_TARGET_AVX512BW)
    ->Args({2, 0, FileType::CSV, EncType::S16_BPD4})
    ->Args({2, 1, FileType::CSV, EncType::S16_BPD4})
    ->Args({2, 2, FileType::CSV, EncType::S16_BPD4})
    ->Args({2, 3, FileType::CSV, EncType::S16_BPD4})
    ->Args({2, 0, FileType::CSV, EncType::S16_BPDM})
    ->Args({2, 1, FileType::CSV, EncType::S16_BPDM})
    ->Args({2, 2, FileType::CSV, EncType::S16_BPDM})
    ->Args({2, 3, FileType::CSV, EncType::S16_BPDM})
#endif
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, index_create)
    ->Args({3, 0, FileType::CSV, EncType::GVB})
    ->Args({3, 1, FileType::CSV, EncType::GVB})
    ->Args({3, 2, FileType::CSV, EncType::GVB})
    ->Args({3, 3, FileType::CSV, EncType::GVB})
    ->Args({3, 0, FileType::CSV, EncType::PEF})
    ->Args({3, 1, FileType::CSV, EncType::PEF})
    ->Args({3, 2, FileType::CSV, EncType::PEF})
    ->Args({3, 3, FileType::CSV, EncType::PEF})
    ->Args({3, 0, FileType::CSV, EncType::FLS})
    ->Args({3, 1, FileType::CSV, EncType::FLS})
    ->Args({3, 2, FileType::CSV, EncType::FLS})
    ->Args({3, 3, FileType::CSV, EncType::FLS})
#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
    ->Args({3, 0, FileType::CSV, EncType::S4_BPD1})
    ->Args({3, 1, FileType::CSV, EncType::S4_BPD1})
    ->Args({3, 2, FileType::CSV, EncType::S4_BPD1})
    ->Args({3, 3, FileType::CSV, EncType::S4_BPD1})
    ->Args({3, 0, FileType::CSV, EncType::S4_BPD2})
    ->Args({3, 1, FileType::CSV, EncType::S4_BPD2})
    ->Args({3, 2, FileType::CSV, EncType::S4_BPD2})
    ->Args({3, 3, FileType::CSV, EncType::S4_BPD2})
    ->Args({3, 0, FileType::CSV, EncType::S4_BPD4})
    ->Args({3, 1, FileType::CSV, EncType::S4_BPD4})
    ->Args({3, 2, FileType::CSV, EncType::S4_BPD4})
    ->Args({3, 3, FileType::CSV, EncType::S4_BPD4})
    ->Args({3, 0, FileType::CSV, EncType::S4_BPDM})
    ->Args({3, 1, FileType::CSV, EncType::S4_BPDM})
    ->Args({3, 2, FileType::CSV, EncType::S4_BPDM})
    ->Args({3, 3, FileType::CSV, EncType::S4_BPDM})
#endif
#if defined(SIMD_TARGET_AVX2)
    ->Args({3, 0, FileType::CSV, EncType::S8_BPD4})
    ->Args({3, 1, FileType::CSV, EncType::S8_BPD4})
    ->Args({3, 2, FileType::CSV, EncType::S8_BPD4})
    ->Args({3, 3, FileType::CSV, EncType::S8_BPD4})
    ->Args({3, 0, FileType::CSV, EncType::S8_BPDM})
    ->Args({3, 1, FileType::CSV, EncType::S8_BPDM})
    ->Args({3, 2, FileType::CSV, EncType::S8_BPDM})
    ->Args({3, 3, FileType::CSV, EncType::S8_BPDM})
#endif
#if defined(SIMD_TARGET_AVX512BW)
    ->Args({3, 0, FileType::CSV, EncType::S16_BPD4})
    ->Args({3, 1, FileType::CSV, EncType::S16_BPD4})
    ->Args({3, 2, FileType::CSV, EncType::S16_BPD4})
    ->Args({3, 3, FileType::CSV, EncType::S16_BPD4})
    ->Args({3, 0, FileType::CSV, EncType::S16_BPDM})
    ->Args({3, 1, FileType::CSV, EncType::S16_BPDM})
    ->Args({3, 2, FileType::CSV, EncType::S16_BPDM})
    ->Args({3, 3, FileType::CSV, EncType::S16_BPDM})
#endif
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, index_create)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 1, FileType::CSV, EncType::GVB})
    ->Args({0, 2, FileType::CSV, EncType::GVB})
    ->Args({0, 3, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({0, 1, FileType::CSV, EncType::PEF})
    ->Args({0, 2, FileType::CSV, EncType::PEF})
    ->Args({0, 3, FileType::CSV, EncType::PEF})
    ->Args({0, 0, FileType::CSV, EncType::FLS})
    ->Args({0, 1, FileType::CSV, EncType::FLS})
    ->Args({0, 2, FileType::CSV, EncType::FLS})
    ->Args({0, 3, FileType::CSV, EncType::FLS})
#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
    ->Args({0, 0, FileType::CSV, EncType::S4_BPD1})
    ->Args({0, 1, FileType::CSV, EncType::S4_BPD1})
    ->Args({0, 2, FileType::CSV, EncType::S4_BPD1})
    ->Args({0, 3, FileType::CSV, EncType::S4_BPD1})
    ->Args({0, 0, FileType::CSV, EncType::S4_BPD2})
    ->Args({0, 1, FileType::CSV, EncType::S4_BPD2})
    ->Args({0, 2, FileType::CSV, EncType::S4_BPD2})
    ->Args({0, 3, FileType::CSV, EncType::S4_BPD2})
    ->Args({0, 0, FileType::CSV, EncType::S4_BPD4})
    ->Args({0, 1, FileType::CSV, EncType::S4_BPD4})
    ->Args({0, 2, FileType::CSV, EncType::S4_BPD4})
    ->Args({0, 3, FileType::CSV, EncType::S4_BPD4})
    ->Args({0, 0, FileType::CSV, EncType::S4_BPDM})
    ->Args({0, 1, FileType::CSV, EncType::S4_BPDM})
    ->Args({0, 2, FileType::CSV, EncType::S4_BPDM})
    ->Args({0, 3, FileType::CSV, EncType::S4_BPDM})
#endif
#if defined(SIMD_TARGET_AVX2)
    ->Args({0, 0, FileType::CSV, EncType::S8_BPD4})
    ->Args({0, 1, FileType::CSV, EncType::S8_BPD4})
    ->Args({0, 2, FileType::CSV, EncType::S8_BPD4})
    ->Args({0, 3, FileType::CSV, EncType::S8_BPD4})
    ->Args({0, 0, FileType::CSV, EncType::S8_BPDM})
    ->Args({0, 1, FileType::CSV, EncType::S8_BPDM})
    ->Args({0, 2, FileType::CSV, EncType::S8_BPDM})
    ->Args({0, 3, FileType::CSV, EncType::S8_BPDM})
#endif
#if defined(SIMD_TARGET_AVX512BW)
    ->Args({0, 0, FileType::CSV, EncType::S16_BPD4})
    ->Args({0, 1, FileType::CSV, EncType::S16_BPD4})
    ->Args({0, 2, FileType::CSV, EncType::S16_BPD4})
    ->Args({0, 3, FileType::CSV, EncType::S16_BPD4})
    ->Args({0, 0, FileType::CSV, EncType::S16_BPDM})
    ->Args({0, 1, FileType::CSV, EncType::S16_BPDM})
    ->Args({0, 2, FileType::CSV, EncType::S16_BPDM})
    ->Args({0, 3, FileType::CSV, EncType::S16_BPDM})
#endif
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, index_create)
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 1, FileType::CSV, EncType::GVB})
    ->Args({1, 2, FileType::CSV, EncType::GVB})
    ->Args({1, 3, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 1, FileType::CSV, EncType::PEF})
    ->Args({1, 2, FileType::CSV, EncType::PEF})
    ->Args({1, 3, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::FLS})
    ->Args({1, 1, FileType::CSV, EncType::FLS})
    ->Args({1, 2, FileType::CSV, EncType::FLS})
    ->Args({1, 3, FileType::CSV, EncType::FLS})
#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
    ->Args({1, 0, FileType::CSV, EncType::S4_BPD1})
    ->Args({1, 1, FileType::CSV, EncType::S4_BPD1})
    ->Args({1, 2, FileType::CSV, EncType::S4_BPD1})
    ->Args({1, 3, FileType::CSV, EncType::S4_BPD1})
    ->Args({1, 0, FileType::CSV, EncType::S4_BPD2})
    ->Args({1, 1, FileType::CSV, EncType::S4_BPD2})
    ->Args({1, 2, FileType::CSV, EncType::S4_BPD2})
    ->Args({1, 3, FileType::CSV, EncType::S4_BPD2})
    ->Args({1, 0, FileType::CSV, EncType::S4_BPD4})
    ->Args({1, 1, FileType::CSV, EncType::S4_BPD4})
    ->Args({1, 2, FileType::CSV, EncType::S4_BPD4})
    ->Args({1, 3, FileType::CSV, EncType::S4_BPD4})
    ->Args({1, 0, FileType::CSV, EncType::S4_BPDM})
    ->Args({1, 1, FileType::CSV, EncType::S4_BPDM})
    ->Args({1, 2, FileType::CSV, EncType::S4_BPDM})
    ->Args({1, 3, FileType::CSV, EncType::S4_BPDM})
#endif
#if defined(SIMD_TARGET_AVX2)
    ->Args({1, 0, FileType::CSV, EncType::S8_BPD4})
    ->Args({1, 1, FileType::CSV, EncType::S8_BPD4})
    ->Args({1, 2, FileType::CSV, EncType::S8_BPD4})
    ->Args({1, 3, FileType::CSV, EncType::S8_BPD4})
    ->Args({1, 0, FileType::CSV, EncType::S8_BPDM})
    ->Args({1, 1, FileType::CSV, EncType::S8_BPDM})
    ->Args({1, 2, FileType::CSV, EncType::S8_BPDM})
    ->Args({1, 3, FileType::CSV, EncType::S8_BPDM})
#endif
#if defined(SIMD_TARGET_AVX512BW)
    ->Args({1, 0, FileType::CSV, EncType::S16_BPD4})
    ->Args({1, 1, FileType::CSV, EncType::S16_BPD4})
    ->Args({1, 2, FileType::CSV, EncType::S16_BPD4})
    ->Args({1, 3, FileType::CSV, EncType::S16_BPD4})
    ->Args({1, 0, FileType::CSV, EncType::S16_BPDM})
    ->Args({1, 1, FileType::CSV, EncType::S16_BPDM})
    ->Args({1, 2, FileType::CSV, EncType::S16_BPDM})
    ->Args({1, 3, FileType::CSV, EncType::S16_BPDM})
#endif
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, index_create)
    ->Args({2, 0, FileType::JSON, EncType::GVB})
    ->Args({2, 1, FileType::JSON, EncType::GVB})
    ->Args({2, 2, FileType::JSON, EncType::GVB})
    ->Args({2, 3, FileType::JSON, EncType::GVB})
    ->Args({2, 0, FileType::JSON, EncType::PEF})
    ->Args({2, 1, FileType::JSON, EncType::PEF})
    ->Args({2, 2, FileType::JSON, EncType::PEF})
    ->Args({2, 3, FileType::JSON, EncType::PEF})
    ->Args({2, 0, FileType::JSON, EncType::FLS})
    ->Args({2, 1, FileType::JSON, EncType::FLS})
    ->Args({2, 2, FileType::JSON, EncType::FLS})
    ->Args({2, 3, FileType::JSON, EncType::FLS})
#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
    ->Args({2, 0, FileType::JSON, EncType::S4_BPD1})
    ->Args({2, 1, FileType::JSON, EncType::S4_BPD1})
    ->Args({2, 2, FileType::JSON, EncType::S4_BPD1})
    ->Args({2, 3, FileType::JSON, EncType::S4_BPD1})
    ->Args({2, 0, FileType::JSON, EncType::S4_BPD2})
    ->Args({2, 1, FileType::JSON, EncType::S4_BPD2})
    ->Args({2, 2, FileType::JSON, EncType::S4_BPD2})
    ->Args({2, 3, FileType::JSON, EncType::S4_BPD2})
    ->Args({2, 0, FileType::JSON, EncType::S4_BPD4})
    ->Args({2, 1, FileType::JSON, EncType::S4_BPD4})
    ->Args({2, 2, FileType::JSON, EncType::S4_BPD4})
    ->Args({2, 3, FileType::JSON, EncType::S4_BPD4})
    ->Args({2, 0, FileType::JSON, EncType::S4_BPDM})
    ->Args({2, 1, FileType::JSON, EncType::S4_BPDM})
    ->Args({2, 2, FileType::JSON, EncType::S4_BPDM})
    ->Args({2, 3, FileType::JSON, EncType::S4_BPDM})
#endif
#if defined(SIMD_TARGET_AVX2)
    ->Args({2, 0, FileType::JSON, EncType::S8_BPD4})
    ->Args({2, 1, FileType::JSON, EncType::S8_BPD4})
    ->Args({2, 2, FileType::JSON, EncType::S8_BPD4})
    ->Args({2, 3, FileType::JSON, EncType::S8_BPD4})
    ->Args({2, 0, FileType::JSON, EncType::S8_BPDM})
    ->Args({2, 1, FileType::JSON, EncType::S8_BPDM})
    ->Args({2, 2, FileType::JSON, EncType::S8_BPDM})
    ->Args({2, 3, FileType::JSON, EncType::S8_BPDM})
#endif
#if defined(SIMD_TARGET_AVX512BW)
    ->Args({2, 0, FileType::JSON, EncType::S16_BPD4})
    ->Args({2, 1, FileType::JSON, EncType::S16_BPD4})
    ->Args({2, 2, FileType::JSON, EncType::S16_BPD4})
    ->Args({2, 3, FileType::JSON, EncType::S16_BPD4})
    ->Args({2, 0, FileType::JSON, EncType::S16_BPDM})
    ->Args({2, 1, FileType::JSON, EncType::S16_BPDM})
    ->Args({2, 2, FileType::JSON, EncType::S16_BPDM})
    ->Args({2, 3, FileType::JSON, EncType::S16_BPDM})
#endif
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, index_create)
    ->Args({3, 0, FileType::JSON, EncType::GVB})
    ->Args({3, 1, FileType::JSON, EncType::GVB})
    ->Args({3, 2, FileType::JSON, EncType::GVB})
    ->Args({3, 3, FileType::JSON, EncType::GVB})
    ->Args({3, 0, FileType::JSON, EncType::PEF})
    ->Args({3, 1, FileType::JSON, EncType::PEF})
    ->Args({3, 2, FileType::JSON, EncType::PEF})
    ->Args({3, 3, FileType::JSON, EncType::PEF})
    ->Args({3, 0, FileType::JSON, EncType::FLS})
    ->Args({3, 1, FileType::JSON, EncType::FLS})
    ->Args({3, 2, FileType::JSON, EncType::FLS})
    ->Args({3, 3, FileType::JSON, EncType::FLS})
#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
    ->Args({3, 0, FileType::JSON, EncType::S4_BPD1})
    ->Args({3, 1, FileType::JSON, EncType::S4_BPD1})
    ->Args({3, 2, FileType::JSON, EncType::S4_BPD1})
    ->Args({3, 3, FileType::JSON, EncType::S4_BPD1})
    ->Args({3, 0, FileType::JSON, EncType::S4_BPD2})
    ->Args({3, 1, FileType::JSON, EncType::S4_BPD2})
    ->Args({3, 2, FileType::JSON, EncType::S4_BPD2})
    ->Args({3, 3, FileType::JSON, EncType::S4_BPD2})
    ->Args({3, 0, FileType::JSON, EncType::S4_BPD4})
    ->Args({3, 1, FileType::JSON, EncType::S4_BPD4})
    ->Args({3, 2, FileType::JSON, EncType::S4_BPD4})
    ->Args({3, 3, FileType::JSON, EncType::S4_BPD4})
    ->Args({3, 0, FileType::JSON, EncType::S4_BPDM})
    ->Args({3, 1, FileType::JSON, EncType::S4_BPDM})
    ->Args({3, 2, FileType::JSON, EncType::S4_BPDM})
    ->Args({3, 3, FileType::JSON, EncType::S4_BPDM})
#endif
#if defined(SIMD_TARGET_AVX2)
    ->Args({3, 0, FileType::JSON, EncType::S8_BPD4})
    ->Args({3, 1, FileType::JSON, EncType::S8_BPD4})
    ->Args({3, 2, FileType::JSON, EncType::S8_BPD4})
    ->Args({3, 3, FileType::JSON, EncType::S8_BPD4})
    ->Args({3, 0, FileType::JSON, EncType::S8_BPDM})
    ->Args({3, 1, FileType::JSON, EncType::S8_BPDM})
    ->Args({3, 2, FileType::JSON, EncType::S8_BPDM})
    ->Args({3, 3, FileType::JSON, EncType::S8_BPDM})
#endif
#if defined(SIMD_TARGET_AVX512BW)
    ->Args({3, 0, FileType::JSON, EncType::S16_BPD4})
    ->Args({3, 1, FileType::JSON, EncType::S16_BPD4})
    ->Args({3, 2, FileType::JSON, EncType::S16_BPD4})
    ->Args({3, 3, FileType::JSON, EncType::S16_BPD4})
    ->Args({3, 0, FileType::JSON, EncType::S16_BPDM})
    ->Args({3, 1, FileType::JSON, EncType::S16_BPDM})
    ->Args({3, 2, FileType::JSON, EncType::S16_BPDM})
    ->Args({3, 3, FileType::JSON, EncType::S16_BPDM})
#endif
    ->Iterations(1);