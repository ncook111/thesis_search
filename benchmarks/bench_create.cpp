#include <benchmark/benchmark.h>
#include <filesystem>
#include "cache_flush.h"
#include "../../src/ingest.h"
#include "../../src/helper.h"

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

BENCHMARK_DEFINE_F(IndexCreate, index_create)(benchmark::State& state) {
    for (auto _ : state) {
        create_mode(index_path, data_path + bench_path, {}, enc_type);
        state.PauseTiming();
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
// BENCHMARK_REGISTER_F(IndexCreate, index_create)
//     ->Args({0, 0, FileType::CSV, EncType::GVB})
//     ->Args({0, 0, FileType::CSV, EncType::PEF})
//     ->Args({1, 0, FileType::CSV, EncType::GVB})
//     ->Args({1, 0, FileType::CSV, EncType::PEF})
//     ->Args({2, 0, FileType::CSV, EncType::GVB})
//     ->Args({2, 0, FileType::CSV, EncType::PEF})
//     ->Args({0, 0, FileType::JSON, EncType::GVB})
//     ->Args({0, 0, FileType::JSON, EncType::PEF})
//     ->Args({1, 0, FileType::JSON, EncType::GVB})
//     ->Args({1, 0, FileType::JSON, EncType::PEF})
//     ->Args({2, 0, FileType::JSON, EncType::GVB})
//     ->Args({2, 0, FileType::JSON, EncType::PEF})
//     ->Iterations(1);

// BENCHMARK_REGISTER_F(IndexCreate, index_create)
//     ->Args({0, 1, FileType::CSV, EncType::GVB})
//     ->Args({0, 1, FileType::CSV, EncType::PEF})
//     ->Args({1, 1, FileType::CSV, EncType::GVB})
//     ->Args({1, 1, FileType::CSV, EncType::PEF})
//     ->Args({2, 1, FileType::CSV, EncType::GVB})
//     ->Args({2, 1, FileType::CSV, EncType::PEF})
//     ->Args({0, 1, FileType::JSON, EncType::GVB})
//     ->Args({0, 1, FileType::JSON, EncType::PEF})
//     ->Args({1, 1, FileType::JSON, EncType::GVB})
//     ->Args({1, 1, FileType::JSON, EncType::PEF})
//     ->Args({2, 1, FileType::JSON, EncType::GVB})
//     ->Args({2, 1, FileType::JSON, EncType::PEF})
//     ->Iterations(1);

// BENCHMARK_REGISTER_F(IndexCreate, index_create)
//     ->Args({0, 2, FileType::CSV, EncType::GVB})
//     ->Args({0, 2, FileType::CSV, EncType::PEF})
//     ->Args({1, 2, FileType::CSV, EncType::GVB})
//     ->Args({1, 2, FileType::CSV, EncType::PEF})
//     ->Args({2, 2, FileType::CSV, EncType::GVB})
//     ->Args({2, 2, FileType::CSV, EncType::PEF})
//     ->Args({0, 2, FileType::JSON, EncType::GVB})
//     ->Args({0, 2, FileType::JSON, EncType::PEF})
//     ->Args({1, 2, FileType::JSON, EncType::GVB})
//     ->Args({1, 2, FileType::JSON, EncType::PEF})
//     ->Args({2, 2, FileType::JSON, EncType::GVB})
//     ->Args({2, 2, FileType::JSON, EncType::PEF})
//     ->Iterations(1);

// BENCHMARK_REGISTER_F(IndexCreate, index_create)
//     ->Args({0, 3, FileType::CSV, EncType::GVB})
//     ->Args({0, 3, FileType::CSV, EncType::PEF})
//     ->Args({1, 3, FileType::CSV, EncType::GVB})
//     ->Args({1, 3, FileType::CSV, EncType::PEF})
//     ->Args({2, 3, FileType::CSV, EncType::GVB})
//     ->Args({2, 3, FileType::CSV, EncType::PEF})
//     ->Args({0, 3, FileType::JSON, EncType::GVB})
//     ->Args({0, 3, FileType::JSON, EncType::PEF})
//     ->Args({1, 3, FileType::JSON, EncType::GVB})
//     ->Args({1, 3, FileType::JSON, EncType::PEF})
//     ->Args({2, 3, FileType::JSON, EncType::GVB})
//     ->Args({2, 3, FileType::JSON, EncType::PEF})
//     ->Iterations(1);