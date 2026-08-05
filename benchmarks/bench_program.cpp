#include <benchmark/benchmark.h>
#include <filesystem>
#include "../../src/ingest.h"
#include "../../src/helper.h"

static std::string csv_data_path = "../benchmarks/bench_data/all_input_data/csv/";
static std::string json_data_path = "../benchmarks/bench_data/all_input_data/json/";
static std::string index_path = "../benchmarks/bench_data/temp_indexes/index_1/";

class IndexCreate : public benchmark::Fixture {
    void SetUp(::benchmark::State& state) {
        std::filesystem::remove_all(index_path);
    }
};

class IndexSearch : public benchmark::Fixture {
    void SetUp(::benchmark::State& state) {
        std::filesystem::remove_all(index_path);
    }
};

BENCHMARK_DEFINE_F(IndexCreate, csv_gvb_create_small_data_1)(benchmark::State& state) {
    std::string bench_path = "small_data/data_1/";
    for (auto _ : state) {
        create_mode(index_path, csv_data_path + bench_path, {}, EncType::GVB);
        state.PauseTiming();
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexCreate, csv_pef_create_small_data_1)(benchmark::State& state) {
    std::string bench_path = "small_data/data_1/";
    for (auto _ : state) {
        create_mode(index_path, csv_data_path + bench_path, {}, EncType::PEF);
        state.PauseTiming();
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexCreate, csv_gvb_create_medium_data_1)(benchmark::State& state) {
    std::string bench_path = "medium_data/data_1/";
    for (auto _ : state) {
        create_mode(index_path, csv_data_path + bench_path, {}, EncType::GVB);
        state.PauseTiming();
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexCreate, csv_pef_create_medium_data_1)(benchmark::State& state) {
    std::string bench_path = "medium_data/data_1/";
    for (auto _ : state) {
        create_mode(index_path, csv_data_path + bench_path, {}, EncType::PEF);
        state.PauseTiming();
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexCreate, csv_gvb_create_large_data_1)(benchmark::State& state) {
    std::string bench_path = "large_data/data_1/";
    for (auto _ : state) {
        create_mode(index_path, csv_data_path + bench_path, {}, EncType::GVB);
        state.PauseTiming();
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexCreate, csv_pef_create_large_data_1)(benchmark::State& state) {
    std::string bench_path = "large_data/data_1/";
    for (auto _ : state) {
        create_mode(index_path, csv_data_path + bench_path, {}, EncType::PEF);
        state.PauseTiming();
        std::filesystem::remove_all(index_path);
        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(IndexCreate, csv_gvb_create_small_data_1)
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, csv_pef_create_small_data_1)
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, csv_gvb_create_medium_data_1)
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, csv_pef_create_medium_data_1)
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, csv_gvb_create_large_data_1)
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexCreate, csv_pef_create_large_data_1)
    ->Iterations(1);

