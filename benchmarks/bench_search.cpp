#include <benchmark/benchmark.h>
#include <filesystem>
#include <mutex>
#include "cache_flush.h"
#include "../src/ingest.h"
#include "../src/helper.h"

constexpr size_t cache_size = 4096 * 1024;

static std::string csv_data_path = BENCH_DATA_DIR "/all_input_data/csv/";
static std::string json_data_path = BENCH_DATA_DIR "/all_input_data/json/";
static std::string logfmt_data_path = BENCH_DATA_DIR "/all_input_data/logfmt/";
static std::string index_path = BENCH_DATA_DIR "/temp_indexes/index_search/";

static double get_percentile(std::vector<double> timings, double percentile) {
  std::sort(timings.begin(), timings.end());
  return timings[static_cast<size_t>(percentile * (timings.size() - 1))];
}

std::vector<std::string> search_types = 
{
    "simple",
    "union",
    "intersection",
    "combination",
    "complex_combination"
};

template <uint8_t FileSize, uint8_t FileType, uint8_t SearchType> static void StandardBenchArgs(benchmark::internal::Benchmark* args) {
    args->Name(std::string("IndexSearch/") + file_size_to_str(FileSize) + '/' + file_type_to_str(FileType) + '/' + search_types[SearchType])
        ->ArgsProduct({
            {FileSize},
            {0, 1, 2, 3},
            {FileType},
            available_enc_types()
        })
        ->Iterations(1)
        ->Unit(benchmark::kMicrosecond)
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
        std::string dataset_size = file_size_to_str(state.range(0));
        dataset_number = state.range(1);
        uint8_t file_type = state.range(2);
        enc_type = state.range(3);
        state.SetLabel(std::string(enc_type_to_str(enc_type) + ',' + dataset_size + "," + file_type_to_str(file_type) + "," + std::to_string(dataset_number)));
        bench_path = file_type_to_str(file_type) + '/' + dataset_size + "_data/data_" + std::to_string(dataset_number + 1) + '_' + enc_type_to_str(enc_type);
        // Initialise required indexes
        std::call_once(wipe_flag, [] { std::filesystem::remove_all(index_path); });
        std::call_once(init_flags[state.range(0)][file_type], [dataset_size, file_type] {
            std::filesystem::create_directories(index_path + file_type_to_str(file_type) + '/' + dataset_size + "_data/");

            std::string data_path;
            switch (file_type) {
                case FileType::CSV:
                    data_path = csv_data_path;
                    break;
                case FileType::JSON:
                    data_path = json_data_path;
                    break;
                case FileType::LOGFMT: default:
                    data_path = logfmt_data_path;
                    break;
            }

            const std::vector<std::string> bench_paths = {
               "_data/data_1",
               "_data/data_2",
               "_data/data_3",
               "_data/data_4",
               "_data/data_5"
            };

            for (auto bench_path : bench_paths) {
                std::string full_data_path = data_path + dataset_size + bench_path;
                std::string full_index_path = index_path + file_type_to_str(file_type) + '/' + dataset_size + bench_path;

                for (auto enc_type : available_enc_types()) {
                    create_mode(full_index_path + '_' + enc_type_to_str(enc_type), full_data_path, {}, enc_type);
                }
            }
        });
    }

    private:
    static std::once_flag wipe_flag;
    static std::once_flag init_flags[3][3];  // Indexed by [file_size][file_type]
};

std::once_flag IndexSearch::wipe_flag;
std::once_flag IndexSearch::init_flags[3][3];

BENCHMARK_DEFINE_F(IndexSearch, simple_search)(benchmark::State& state) {
    std::string query = queries[0][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query);
        benchmark::ClobberMemory();

    }
}

BENCHMARK_DEFINE_F(IndexSearch, union_search)(benchmark::State& state) {
    std::string query = queries[1][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, intersection_search)(benchmark::State& state) {
    std::string query = queries[2][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, combination_search)(benchmark::State& state) {
    std::string query = queries[3][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_DEFINE_F(IndexSearch, complex_combination_search)(benchmark::State& state) {
    std::string query = queries[4][dataset_number];
    for (auto _ : state) {
        state.PauseTiming();
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query);
        benchmark::ClobberMemory();
    }
}

BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 0>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 1>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 2>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 3>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 4>);
