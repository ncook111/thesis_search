#include <benchmark/benchmark.h>
#include <filesystem>
#include "cache_flush.h"
#include "../../src/ingest.h"
#include "../../src/helper.h"

constexpr size_t cache_size = 4096 * 1024;

static std::string csv_data_path = "../benchmarks/bench_data/all_input_data/csv/";
static std::string json_data_path = "../benchmarks/bench_data/all_input_data/json/";
static std::string index_path = "../benchmarks/bench_data/temp_indexes/index_search/";
static std::string temp_output_path = "../benchmarks/bench_data/temp_indexes/";

static const std::vector<std::string> datasets = {"small", "medium", "large"};

static const std::vector<std::string> bench_paths = {"small_data/data_1", "small_data/data_2", "small_data/data_3", "small_data/data_4", "medium_data/data_1", "medium_data/data_2", "medium_data/data_3", "medium_data/data_4", "large_data/data_1", "large_data/data_2", "large_data/data_3", "large_data/data_4"};

static const std::vector<std::vector<std::string>> queries = {
        // simple_search
        {"[*] = \"male\"", "[*] = \"1994-01-01\"", "[*] = \"Pavel\"", "[*] = \"2025-09-30\""},

}; 

class IndexSearch : public benchmark::Fixture {
public:
    int dataset_number;
    std::string bench_path;
    std::string data_path;
    uint8_t enc_type;
    void SetUp(::benchmark::State& state) {
        int dataset_size = state.range(0);
        dataset_number = state.range(1);
        uint8_t file_type = state.range(2);
        enc_type = state.range(3);
        std::string dataset = datasets[dataset_size];
        state.SetLabel(std::to_string(dataset_number + 1) + "_" + dataset + "/" + enc_type_to_str(enc_type));
        bench_path = dataset + "_data/data_" + std::to_string(dataset_number+1) + "_" + enc_type_to_str(enc_type);
        data_path = dataset + "_data/data_" + std::to_string(dataset_number+1);
        // Initialise all required indexes at once
        std::call_once(init_flag, [] {
            // std::filesystem::remove_all(index_path);
            // std::filesystem::create_directory(index_path);
            // std::filesystem::create_directory(index_path + "small_data");
            // std::filesystem::create_directory(index_path + "medium_data");
            // std::filesystem::create_directory(index_path + "large_data");

            for (auto bench_path : bench_paths) {
                create_mode(index_path + bench_path + "_gvb", csv_data_path + bench_path, {}, EncType::GVB);
                create_mode(index_path + bench_path + "_pef", csv_data_path + bench_path, {}, EncType::PEF);
            }
        });
    }

    void TearDown(::benchmark::State& state) {
        // std::filesystem::remove(temp_output_path+"out.txt");
    }

    private:
    static std::once_flag init_flag;
};

std::once_flag IndexSearch::init_flag;

BENCHMARK_DEFINE_F(IndexSearch, simple_search)(benchmark::State& state) {
    std::string query = queries[0][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", "", query, enc_type);
        benchmark::ClobberMemory();

    }
}

/*
    Arg1: dataset size
    Arg2: dataset number
    Arg3: file type
    Arg4: encoding
*/
BENCHMARK_REGISTER_F(IndexSearch, simple_search)
    ->Args({2, 0, FileType::CSV, EncType::GVB})
    ->Args({2, 1, FileType::CSV, EncType::GVB})
    ->Args({2, 2, FileType::CSV, EncType::GVB})
    ->Args({2, 3, FileType::CSV, EncType::GVB})
    ->Args({2, 0, FileType::CSV, EncType::PEF})
    ->Args({2, 1, FileType::CSV, EncType::PEF})
    ->Args({2, 2, FileType::CSV, EncType::PEF})
    ->Args({2, 3, FileType::CSV, EncType::PEF})
    ->Iterations(50);