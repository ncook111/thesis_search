#include <benchmark/benchmark.h>
#include <filesystem>
#include "cache_flush.h"
#include "../src/ingest.h"
#include "../src/helper.h"

constexpr size_t cache_size = 4096 * 1024;

static std::string csv_data_path = BENCH_DATA_DIR "/all_input_data/csv/";
static std::string json_data_path = BENCH_DATA_DIR "/all_input_data/json/";
static std::string logfmt_data_path = BENCH_DATA_DIR "/all_input_data/logfmt/";
static std::string index_path = BENCH_DATA_DIR "/temp_indexes/index_1";

static const std::vector<std::string> datasets = {"small", "medium", "large"};

template <uint8_t FileSize, uint8_t FileType> static void StandardBenchArgs(benchmark::internal::Benchmark* args) {
    args->Name(std::string("IndexCreate/") + file_size_to_str(FileSize) + "/" + file_type_to_str(FileType))
        ->ArgNames({"file_size", "dataset", "file_type", "scheme"})
        ->ArgsProduct({
            {FileSize},
            {0, 1, 2, 3, 4},
            {FileType},
            available_enc_types()
        })
        ->Unit(benchmark::kSecond)
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
        std::filesystem::create_directories(std::filesystem::path(index_path).parent_path());
        std::string dataset = datasets[dataset_size];
        state.SetLabel(std::string(enc_type_to_str(enc_type) + ',' + file_size_to_str(dataset_size) + "," + file_type_to_str(file_type) + "," + std::to_string(dataset_number)));
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
        state.counters["index_size"]   = index_size;
        state.counters["data_size"]   = data_size;
        state.counters["compression"] = (float)index_size / (float)data_size;
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}


BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT>);
BENCHMARK_REGISTER_F(IndexCreate, index_create)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT>);
