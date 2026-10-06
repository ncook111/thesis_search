#include <benchmark/benchmark.h>
#include <filesystem>
#include "cache_flush.h"
#include "../src/ingest.h"
#include "../src/helper.h"

constexpr size_t cache_size = 4096 * 1024;

static std::string csv_data_path = "../benchmarks/bench_data/all_input_data/csv/";
static std::string json_data_path = "../benchmarks/bench_data/all_input_data/json/";
static std::string logfmt_data_path = "../benchmarks/bench_data/all_input_data/logfmt/";
static std::string index_path = "../benchmarks/bench_data/temp_indexes/index_1/";

static const std::vector<std::string> datasets = {"small", "medium", "large"};

namespace FileSize {
enum Value : uint8_t { SMALL = 0, MEDIUM = 1, LARGE = 2 };
}

/*
    Arg1: dataset size
    Arg2: dataset number
    Arg3: file type
    Arg4: encoding
*/
template <uint8_t FileSize, uint8_t FileType> static void StandardBenchArgs(benchmark::internal::Benchmark* args) {
    args->Args({FileSize, 0, FileType, EncType::GVB})
        ->Args({FileSize, 1, FileType, EncType::GVB})
        ->Args({FileSize, 2, FileType, EncType::GVB})
        ->Args({FileSize, 3, FileType, EncType::GVB})
        ->Args({FileSize, 0, FileType, EncType::PEF})
        ->Args({FileSize, 1, FileType, EncType::PEF})
        ->Args({FileSize, 2, FileType, EncType::PEF})
        ->Args({FileSize, 3, FileType, EncType::PEF})
        ->Args({FileSize, 0, FileType, EncType::FLS})
        ->Args({FileSize, 1, FileType, EncType::FLS})
        ->Args({FileSize, 2, FileType, EncType::FLS})
        ->Args({FileSize, 3, FileType, EncType::FLS})
    #if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
        ->Args({FileSize, 0, FileType, EncType::S4_BPD1})
        ->Args({FileSize, 1, FileType, EncType::S4_BPD1})
        ->Args({FileSize, 2, FileType, EncType::S4_BPD1})
        ->Args({FileSize, 3, FileType, EncType::S4_BPD1})
        ->Args({FileSize, 0, FileType, EncType::S4_BPD2})
        ->Args({FileSize, 1, FileType, EncType::S4_BPD2})
        ->Args({FileSize, 2, FileType, EncType::S4_BPD2})
        ->Args({FileSize, 3, FileType, EncType::S4_BPD2})
        ->Args({FileSize, 0, FileType, EncType::S4_BPD4})
        ->Args({FileSize, 1, FileType, EncType::S4_BPD4})
        ->Args({FileSize, 2, FileType, EncType::S4_BPD4})
        ->Args({FileSize, 3, FileType, EncType::S4_BPD4})
        ->Args({FileSize, 0, FileType, EncType::S4_BPDM})
        ->Args({FileSize, 1, FileType, EncType::S4_BPDM})
        ->Args({FileSize, 2, FileType, EncType::S4_BPDM})
        ->Args({FileSize, 3, FileType, EncType::S4_BPDM})
    #endif
    #if defined(SIMD_TARGET_AVX2)
        ->Args({FileSize, 0, FileType, EncType::S8_BPD4})
        ->Args({FileSize, 1, FileType, EncType::S8_BPD4})
        ->Args({FileSize, 2, FileType, EncType::S8_BPD4})
        ->Args({FileSize, 3, FileType, EncType::S8_BPD4})
        ->Args({FileSize, 0, FileType, EncType::S8_BPDM})
        ->Args({FileSize, 1, FileType, EncType::S8_BPDM})
        ->Args({FileSize, 2, FileType, EncType::S8_BPDM})
        ->Args({FileSize, 3, FileType, EncType::S8_BPDM})
    #endif
    #if defined(SIMD_TARGET_AVX512BW)
        ->Args({FileSize, 0, FileType, EncType::S16_BPD4})
        ->Args({FileSize, 1, FileType, EncType::S16_BPD4})
        ->Args({FileSize, 2, FileType, EncType::S16_BPD4})
        ->Args({FileSize, 3, FileType, EncType::S16_BPD4})
        ->Args({FileSize, 0, FileType, EncType::S16_BPDM})
        ->Args({FileSize, 1, FileType, EncType::S16_BPDM})
        ->Args({FileSize, 2, FileType, EncType::S16_BPDM})
        ->Args({FileSize, 3, FileType, EncType::S16_BPDM})
    #endif
        ->Iterations(1);
}
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
        
        switch (file_type) {
            case FileType::CSV:
                data_path = csv_data_path;
                break;
            case FileType::JSON:
                data_path = json_data_path;
                break;
            case FileType::LOGFMT:
                data_path = logfmt_data_path;
                break;
        }
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


BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV>);
// BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV>);
// BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON>);
// BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON>);
// BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON>);

/*
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT>);
*/