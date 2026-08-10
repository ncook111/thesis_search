#include <benchmark/benchmark.h>
#include <filesystem>
#include "cache_flush.h"
#include "../../src/ingest.h"
#include "../../src/helper.h"

constexpr size_t cache_size = 4096 * 1024;

static std::string csv_data_path = "../benchmarks/bench_data/all_input_data/csv/";
static std::string json_data_path = "../benchmarks/bench_data/all_input_data/json/";
static std::string index_path = "../benchmarks/bench_data/temp_indexes/index_search/";

static const std::vector<std::string> datasets = {"small", "medium", "large"};

static const std::vector<std::string> bench_paths = {"small_data/data_1", "small_data/data_2", "small_data/data_3", "small_data/data_4", "medium_data/data_1", "medium_data/data_2", "medium_data/data_3", "medium_data/data_4", "large_data/data_1", "large_data/data_2", "large_data/data_3", "large_data/data_4"};

static const std::vector<std::vector<std::string>> queries = {
        // simple_search
        {"[sex] = \"male\"", "[dob] = {1994-01-01,1995-12-31}", "[authors] = \"Pavel \"]"},

        // union_search
        {"[sex] = \"male\" | [sex] = \"female\"", "[dob] = {1994-01-01,1995-12-31} | [dob] = {1984-01-01,1985-12-31}", "[authors] = \"Pavel\"] | [authors] = \"Louis\"]"},

        // intersection_search
        {"[sex] = \"male\" & [first_name] = \"bob\"", "[dob] = {1994-01-01,1995-12-31} & [acc_bal] = {0.1,2000.1}", "[authors] = \"Pavel\"] & [authors] = \"Dimitry\"]"},

        // combination_search
        {"([sex] = \"male\" | [sex] = \"female\") & [first_name] = \"bob\"", "([dob] = {1994-01-01,1995-12-31} | [dob] = {1984-01-01,1985-12-31}) & [acc_bal] = {0.1,2000.1}", "([authors] = \"Pavel\"] | [authors] = \"Dimitry\"]) & [title] = \"Graph Decompositions\""},

        // complex combination_search
        {"([sex] = \"male\" & [first_name] = \"bob\") | ([sex] = \"female\" & [first_name] = \"Gloria\" & ![city] = \"Kingsport\")", "([dob] = {1994-01-01,1995-12-31} | [dob] = {1984-01-01,1985-12-31}) & [acc_bal] = {0.1,2000.1} & ![acc_num] = {0,74578716}", "([authors] = \"Pavel\"] | [authors] = \"Dimitry\"] | [authors] = \"Alejandro\"]) & ([title] = \"Graph Decompositions\" | [title] = \"prompt diphoton\") & ![authors] = \"Berger\"]"}
}; 

class IndexSearch : public benchmark::Fixture {
public:
    int dataset_number;
    std::string bench_path;
    uint8_t enc_type;
    void SetUp(::benchmark::State& state) {
        int dataset_size = state.range(0);
        dataset_number = state.range(1);
        uint8_t file_type = state.range(2);
        enc_type = state.range(3);
        std::string dataset = datasets[dataset_size];
        state.SetLabel(std::to_string(dataset_number + 1) + "_" + dataset + "/" + enc_type_to_str(enc_type));
        bench_path = dataset + "_data/data_" + std::to_string(dataset_number+1) + "_" + enc_type_to_str(enc_type);
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

    private:
    static std::once_flag init_flag;
};

std::once_flag IndexSearch::init_flag;

BENCHMARK_DEFINE_F(IndexSearch, simple_search)(benchmark::State& state) {
    std::string query = queries[0][dataset_number];
    for (auto _ : state) {
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, union_search)(benchmark::State& state) {
    std::string query = queries[1][dataset_number];
    for (auto _ : state) {
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, intersection_search)(benchmark::State& state) {
    std::string query = queries[2][dataset_number];
    for (auto _ : state) {
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, combination_search)(benchmark::State& state) {
    std::string query = queries[3][dataset_number];
    for (auto _ : state) {
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, complex_combination_search)(benchmark::State& state) {
    std::string query = queries[4][dataset_number];
    for (auto _ : state) {
        search_mode(index_path + bench_path, "", {}, query, enc_type);
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
// Dataset 1
BENCHMARK_REGISTER_F(IndexSearch, simple_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({2, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Args({2, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, union_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({2, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Args({2, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, intersection_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({2, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Args({2, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, combination_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({2, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Args({2, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)
    ->Args({0, 0, FileType::CSV, EncType::GVB})
    ->Args({1, 0, FileType::CSV, EncType::GVB})
    ->Args({2, 0, FileType::CSV, EncType::GVB})
    ->Args({0, 0, FileType::CSV, EncType::PEF})
    ->Args({1, 0, FileType::CSV, EncType::PEF})
    ->Args({2, 0, FileType::CSV, EncType::PEF})
    ->Iterations(50);

// Dataset 2
    BENCHMARK_REGISTER_F(IndexSearch, simple_search)
    ->Args({0, 1, FileType::CSV, EncType::GVB})
    ->Args({1, 1, FileType::CSV, EncType::GVB})
    ->Args({2, 1, FileType::CSV, EncType::GVB})
    ->Args({0, 1, FileType::CSV, EncType::PEF})
    ->Args({1, 1, FileType::CSV, EncType::PEF})
    ->Args({2, 1, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, union_search)
    ->Args({0, 1, FileType::CSV, EncType::GVB})
    ->Args({1, 1, FileType::CSV, EncType::GVB})
    ->Args({2, 1, FileType::CSV, EncType::GVB})
    ->Args({0, 1, FileType::CSV, EncType::PEF})
    ->Args({1, 1, FileType::CSV, EncType::PEF})
    ->Args({2, 1, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, intersection_search)
    ->Args({0, 1, FileType::CSV, EncType::GVB})
    ->Args({1, 1, FileType::CSV, EncType::GVB})
    ->Args({2, 1, FileType::CSV, EncType::GVB})
    ->Args({0, 1, FileType::CSV, EncType::PEF})
    ->Args({1, 1, FileType::CSV, EncType::PEF})
    ->Args({2, 1, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, combination_search)
    ->Args({0, 1, FileType::CSV, EncType::GVB})
    ->Args({1, 1, FileType::CSV, EncType::GVB})
    ->Args({2, 1, FileType::CSV, EncType::GVB})
    ->Args({0, 1, FileType::CSV, EncType::PEF})
    ->Args({1, 1, FileType::CSV, EncType::PEF})
    ->Args({2, 1, FileType::CSV, EncType::PEF})
    ->Iterations(50);

BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)
    ->Args({0, 1, FileType::CSV, EncType::GVB})
    ->Args({1, 1, FileType::CSV, EncType::GVB})
    ->Args({2, 1, FileType::CSV, EncType::GVB})
    ->Args({0, 1, FileType::CSV, EncType::PEF})
    ->Args({1, 1, FileType::CSV, EncType::PEF})
    ->Args({2, 1, FileType::CSV, EncType::PEF})
    ->Iterations(50);

// Dataset 3
BENCHMARK_REGISTER_F(IndexSearch, simple_search)
    ->Args({0, 2, FileType::CSV, EncType::GVB})
    ->Args({1, 2, FileType::CSV, EncType::GVB})
    ->Args({2, 2, FileType::CSV, EncType::GVB})
    ->Args({0, 2, FileType::CSV, EncType::PEF})
    ->Args({1, 2, FileType::CSV, EncType::PEF})
    ->Args({2, 2, FileType::CSV, EncType::PEF})
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexSearch, union_search)
    ->Args({0, 2, FileType::CSV, EncType::GVB})
    ->Args({1, 2, FileType::CSV, EncType::GVB})
    ->Args({2, 2, FileType::CSV, EncType::GVB})
    ->Args({0, 2, FileType::CSV, EncType::PEF})
    ->Args({1, 2, FileType::CSV, EncType::PEF})
    ->Args({2, 2, FileType::CSV, EncType::PEF})
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexSearch, intersection_search)
    ->Args({0, 2, FileType::CSV, EncType::GVB})
    ->Args({1, 2, FileType::CSV, EncType::GVB})
    ->Args({2, 2, FileType::CSV, EncType::GVB})
    ->Args({0, 2, FileType::CSV, EncType::PEF})
    ->Args({1, 2, FileType::CSV, EncType::PEF})
    ->Args({2, 2, FileType::CSV, EncType::PEF})
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexSearch, combination_search)
    ->Args({0, 2, FileType::CSV, EncType::GVB})
    ->Args({1, 2, FileType::CSV, EncType::GVB})
    ->Args({2, 2, FileType::CSV, EncType::GVB})
    ->Args({0, 2, FileType::CSV, EncType::PEF})
    ->Args({1, 2, FileType::CSV, EncType::PEF})
    ->Args({2, 2, FileType::CSV, EncType::PEF})
    ->Iterations(1);

BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)
    ->Args({0, 2, FileType::CSV, EncType::GVB})
    ->Args({0, 2, FileType::CSV, EncType::PEF})
    ->Args({1, 2, FileType::CSV, EncType::GVB})
    ->Args({1, 2, FileType::CSV, EncType::PEF})
    ->Args({2, 2, FileType::CSV, EncType::GVB})
    ->Args({2, 2, FileType::CSV, EncType::PEF})
    ->Iterations(1);

// Dataset 4
// BENCHMARK_REGISTER_F(IndexSearch, simple_search)
//     ->Args({0, 3, FileType::CSV, EncType::GVB})
//     ->Args({0, 3, FileType::CSV, EncType::PEF})
//     ->Args({1, 3, FileType::CSV, EncType::GVB})
//     ->Args({1, 3, FileType::CSV, EncType::PEF})
//     ->Args({2, 3, FileType::CSV, EncType::GVB})
//     ->Args({2, 3, FileType::CSV, EncType::PEF})
//     ->Iterations(1);

// BENCHMARK_REGISTER_F(IndexSearch, union_search)
//     ->Args({0, 3, FileType::CSV, EncType::GVB})
//     ->Args({0, 3, FileType::CSV, EncType::PEF})
//     ->Args({1, 3, FileType::CSV, EncType::GVB})
//     ->Args({1, 3, FileType::CSV, EncType::PEF})
//     ->Args({2, 3, FileType::CSV, EncType::GVB})
//     ->Args({2, 3, FileType::CSV, EncType::PEF})
//     ->Iterations(1);

// BENCHMARK_REGISTER_F(IndexSearch, intersection_search)
//     ->Args({0, 3, FileType::CSV, EncType::GVB})
//     ->Args({0, 3, FileType::CSV, EncType::PEF})
//     ->Args({1, 3, FileType::CSV, EncType::GVB})
//     ->Args({1, 3, FileType::CSV, EncType::PEF})
//     ->Args({2, 3, FileType::CSV, EncType::GVB})
//     ->Args({2, 3, FileType::CSV, EncType::PEF})
//     ->Iterations(1);

// BENCHMARK_REGISTER_F(IndexSearch, combination_search)
//     ->Args({0, 3, FileType::CSV, EncType::GVB})
//     ->Args({0, 3, FileType::CSV, EncType::PEF})
//     ->Args({1, 3, FileType::CSV, EncType::GVB})
//     ->Args({1, 3, FileType::CSV, EncType::PEF})
//     ->Args({2, 3, FileType::CSV, EncType::GVB})
//     ->Args({2, 3, FileType::CSV, EncType::PEF})
//     ->Iterations(1);

// BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)
//     ->Args({0, 3, FileType::CSV, EncType::GVB})
//     ->Args({0, 3, FileType::CSV, EncType::PEF})
//     ->Args({1, 3, FileType::CSV, EncType::GVB})
//     ->Args({1, 3, FileType::CSV, EncType::PEF})
//     ->Args({2, 3, FileType::CSV, EncType::GVB})
//     ->Args({2, 3, FileType::CSV, EncType::PEF})
//     ->Iterations(1);