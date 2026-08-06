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
    void SetUp(::benchmark::State& state) {
        std::filesystem::remove_all(index_path);
        std::string dataset = datasets[state.range(0)];
        state.SetLabel(std::to_string(state.range(1)) + "_" + dataset + "/" + enc_type_to_str(state.range(3)));
        bench_path = dataset + "_data/data_1/";
        data_path = (state.range(2) == FileType::CSV) ? csv_data_path : json_data_path;
    }
};

class IndexSearch : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& state) {
        std::filesystem::remove_all(index_path);
        std::string dataset = datasets[state.range(0)];
        state.SetLabel(std::to_string(state.range(1)) + "_" + dataset + "/" + enc_type_to_str(state.range(3)));
        std::string bench_path = dataset + "_data/data_1/";
        std::string data_path = (state.range(2) == FileType::CSV) ? csv_data_path : json_data_path;
        create_mode(index_path, data_path + bench_path, {}, state.range(1));
    }
};

BENCHMARK_DEFINE_F(IndexCreate, index_create)(benchmark::State& state) {
    for (auto _ : state) {
        create_mode(index_path, csv_data_path + bench_path, {}, state.range(1));
        state.PauseTiming();
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, simple_search)(benchmark::State& state) {
    std::string query = "[*] = \"male\"";
    for (auto _ : state) {
        search_mode(index_path, "", {}, query, state.range(1));
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, union_search)(benchmark::State& state) {
    std::string query = "[*] = \"male\" | [*] = \"female\"";
    for (auto _ : state) {
        search_mode(index_path, "", {}, query, state.range(1));
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, intersection_search)(benchmark::State& state) {
    std::string query = "[*] = \"male\" & [*] = \"bob\"";
    for (auto _ : state) {
        search_mode(index_path, "", {}, query, state.range(1));
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, combination_search)(benchmark::State& state) {
    std::string query = "([*] = \"male\" | [*] = \"female\") & [*] = \"bob\"";
    for (auto _ : state) {
        search_mode(index_path, "", {}, query, state.range(1));
        state.PauseTiming();
        flush_cache(cache_size);
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
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Args({2, 0, FileType::CSV, EncType::GVB})
    ->Args({2, 0, FileType::CSV, EncType::PEF})
    ->Args({0, 1, FileType::JSON, EncType::GVB})
    ->Args({0, 1, FileType::JSON, EncType::PEF})
    ->Args({1, 1, FileType::JSON, EncType::GVB})
    ->Args({1, 1, FileType::JSON, EncType::PEF})
    ->Args({2, 1, FileType::JSON, EncType::GVB})
    ->Args({2, 1, FileType::JSON, EncType::PEF})
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexSearch, simple_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, union_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, intersection_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, combination_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);