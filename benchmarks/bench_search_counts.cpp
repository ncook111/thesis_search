#include <benchmark/benchmark.h>
#include <filesystem>
#include <mutex>
#include "cache_flush.h"
#include "../src/ingest.h"
#include "../src/helper.h"

#define NUM_REPETITIONS 100
constexpr size_t cache_size = 4096 * 1024;

static std::string csv_data_path = "../benchmarks/bench_data/all_input_data/csv/";
static std::string json_data_path = "../benchmarks/bench_data/all_input_data/json/";
static std::string index_path = "../benchmarks/bench_data/temp_indexes/index_search/";

static const std::vector<std::string> datasets = {"small", "medium", "large"};

static const std::vector<std::string> bench_paths = {"small_data/data_1","small_data/data_2", "small_data/data_3", "small_data/data_4", /*"medium_data/data_1", "medium_data/data_2", "medium_data/data_3", "medium_data/data_4", "large_data/data_1", "large_data/data_2", "large_data/data_3", "large_data/data_4"*/};

static double get_percentile(std::vector<double> timings, double percentile) {
  std::sort(timings.begin(), timings.end());
  return timings[static_cast<size_t>(percentile * (timings.size() - 1))];
}

// /*
//     Arg1: dataset size
//     Arg2: dataset number
//     Arg3: file type
//     Arg4: encoding
// */
static void StandardBenchArgs(benchmark::internal::Benchmark* args) {
    args->Args({0, 0, FileType::CSV, EncType::GVB})
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
        ->Iterations(1)
        ->Unit(benchmark::kMicrosecond)
        ->Repetitions(NUM_REPETITIONS)
        ->ComputeStatistics("p90", [](const std::vector<double>& v) { return get_percentile(v, 0.90); })
        ->ComputeStatistics("p99", [](const std::vector<double>& v) { return get_percentile(v, 0.99); })
        ->ComputeStatistics("max", [](const std::vector<double>& v) { return *std::max_element(v.begin(), v.end()); });
}

static const std::vector<std::vector<std::string>> queries = {
        // simple_search
        {
            "[*] = \"male\"", 
            "[*] = \"1994-01-01\"", 
            "[*] = \"Pavel\"", 
            "[*] = \"53\""
        },

        // union_search
        {
            "[sex] = \"male\" | [sex] = \"female\"",
            "[dob] = {1994-01-01,1995-12-31} | [dob] = {1984-01-01,1985-12-31}",
            "[authors] = \"Pavel\" | [authors] = \"Louis\"",
            "[time_stamp] = \"2025-09-30\" | [time_stamp] = \"2025-10-01\""
        },

        // intersection_search
        {
            "[sex] = \"male\" & [first_name] = \"bob\"",
            "[dob] = {1994-01-01,1995-12-31} & [acc_bal] = {0.1,2000.1}",
            "[authors] = \"Pavel\" & [authors] = \"Dimitry\"",
            "[time_stamp] = \"2025-09-30\" & [time_stamp] = \"07:00:00\""
        },

        // combination_search
        {
            "([sex] = \"male\" | [sex] = \"female\") & [first_name] = \"bob\"",
            "([dob] = {1994-01-01,1995-12-31} | [dob] = {1984-01-01,1985-12-31}) & [acc_bal] = {0.1,2000.1}",
            "([authors] = \"Pavel\" | [authors] = \"Louis\") & [title] = \"Graph Decompositions\"",
            "([time_stamp] = \"2025-09-30\" | [time_stamp] = \"2025-10-01\") | [time_stamp] = \"07:00:00\""
        },

        // complex combination_search
        {
            "([sex] = \"male\" & [first_name] = \"bob\") | ([sex] = \"female\" & [first_name] = \"Gloria\" & ![city] = \"Kingsport\")",
            "([dob] = {1994-01-01,1995-12-31} | [dob] = {1984-01-01,1985-12-31}) & [acc_bal] = {0.1,2000.1} & ![acc_num] = \"74578716\"",
            "([authors] = \"Pavel\" | [authors] = \"Dimitry\" | [authors] = \"Alejandro\") & ([title] = \"Graph Decompositions\" | [title] = \"prompt diphoton\") & ![authors] = \"Berger\"",
            "([time_stamp] = \"2025-09-30\" & [time_stamp] = \"07:00:00\") | ([time_stamp] = \"2025-10-01\" | [time_stamp] = \"07:00:00\") & [sensor_0_max] = {,63.0}"
        }
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
            std::filesystem::remove_all(index_path);
            std::filesystem::create_directory(index_path);
            std::filesystem::create_directory(index_path + "small_data");
            std::filesystem::create_directory(index_path + "medium_data");
            std::filesystem::create_directory(index_path + "large_data");

            for (auto bench_path : bench_paths) {
                create_mode(index_path + bench_path + "_gvb", csv_data_path + bench_path, {}, EncType::GVB);
                create_mode(index_path + bench_path + "_pef", csv_data_path + bench_path, {}, EncType::PEF);
                create_mode(index_path + bench_path + "_fls", csv_data_path + bench_path, {}, EncType::FLS);
            #if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)
                create_mode(index_path + bench_path + "_s4_bpd1", csv_data_path + bench_path, {}, EncType::S4_BPD1);
                create_mode(index_path + bench_path + "_s4_bpd2", csv_data_path + bench_path, {}, EncType::S4_BPD2);
                create_mode(index_path + bench_path + "_s4_bpd4", csv_data_path + bench_path, {}, EncType::S4_BPD4);
                create_mode(index_path + bench_path + "_s4_bpdm", csv_data_path + bench_path, {}, EncType::S4_BPDM);
            #endif
            #if defined(SIMD_TARGET_AVX2)
                create_mode(index_path + bench_path + "_s8_bpd4", csv_data_path + bench_path, {}, EncType::S8_BPD4);
                create_mode(index_path + bench_path + "_s8_bpdm", csv_data_path + bench_path, {}, EncType::S8_BPDM);
            #endif
            #if defined(SIMD_TARGET_AVX512BW)
                create_mode(index_path + bench_path + "_s16_bpd4", csv_data_path + bench_path, {}, EncType::S16_BPD4);
                create_mode(index_path + bench_path + "_s16_bpdm", csv_data_path + bench_path, {}, EncType::S16_BPDM);
            #endif
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
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        benchmark::ClobberMemory();

    }
}

BENCHMARK_DEFINE_F(IndexSearch, union_search)(benchmark::State& state) {
    std::string query = queries[1][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, intersection_search)(benchmark::State& state) {
    std::string query = queries[2][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, combination_search)(benchmark::State& state) {
    std::string query = queries[3][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, complex_combination_search)(benchmark::State& state) {
    std::string query = queries[4][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, enc_type);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs);