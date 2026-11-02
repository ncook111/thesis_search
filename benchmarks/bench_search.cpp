#include <benchmark/benchmark.h>
#include <filesystem>
#include <mutex>
#include <cstring>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include "cache_flush.h"
#include "../src/ingest.h"
#include "../src/helper.h"

constexpr size_t cache_size = 2048 * 1024;

static std::string csv_data_path = BENCH_DATA_DIR "/all_input_data/csv/";
static std::string json_data_path = BENCH_DATA_DIR "/all_input_data/json/";
static std::string logfmt_data_path = BENCH_DATA_DIR "/all_input_data/logfmt/";
static std::string index_path = BENCH_DATA_DIR "/temp_indexes/index_search/";

static void reset_rss_hwm() { std::ofstream("/proc/self/clear_refs") << "5"; }

static size_t read_rss_kb(const char* key) {
    std::ifstream f("/proc/self/status");
    for (std::string line; std::getline(f, line);)
        if (line.rfind(key, 0) == 0) return std::stoul(line.substr(std::strlen(key)));
    return 0;
}

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
        ->UseRealTime()
        ->Unit(benchmark::kMicrosecond)
        ->ComputeStatistics("p95", [](const std::vector<double>& v) { return get_percentile(v, 0.95); })
        ->ComputeStatistics("max", [](const std::vector<double>& v) { return *std::max_element(v.begin(), v.end()); });
}

static const std::vector<std::vector<std::string>> queries = {
        // simple_search
        {
            "[reviewtext] = \"book\"", 
            "[type] = \"IssueCommentEvent\"", 
            "[page_title] = \"germany\"", 
            "[publication_year] = \"2026\""
        },

        // union_search
        {
            "[reviewtext] = \"book\" | [reviewtext] = \"television\"",
            "[type] = \"IssueCommentEvent\" | [type] = \"PullRequestReviewCommentEvent\"",
            "[page_title] = \"germany\" | [page_title] = \"england\"",
            "[publication_year] = \"2026\" | [publication_year] = \"2025\""
        },

        // intersection_search
        {
            "[reviewtext] = \"book\" & [reviewtext] = \"paperback\"",
            "[type] = \"IssueCommentEvent\" & [created_at] = \"2026-01-01\"",
            "[timestamp] = \"2026-01-01\" & [page_title] = \"germany\"",
            "[publication_year] = \"2026\" & [language] = \"en\""
        },

        // combination_search
        {
            "([overall] = \"5.0\" & [reviewtext] = \"book\") & [reviewtext] = \"paperback\"",
            "([type] = \"IssueCommentEvent\" | [type] = \"PullRequestReviewCommentEvent\") & [created_at] = \"2026-01-01\"",
            "([page_title] = \"germany\" | [page_title] = \"england\") | [timestamp] = \"2026-01-01\"",
            "([publication_year] = \"2026\" | [publication_year] = \"2025\") & [language] = \"en\""
        },

        // complex combination_search
        {
            "([overall] = \"5.0\" & [reviewtext] = \"paperback\") | ([overall] = {2.0,4.0} & [reviewtext] = \"television\" & ![reviewtext] = \"terrible\")",
            "([type] = \"IssueCommentEvent\" | [type] = \"PullRequestReviewCommentEvent\") & [created_at] = {2025-01-01, 2026-01-01} & ![created_at] = \"2025-12-25\"",
            "([page_title] = \"germany\" | [page_title] = \"england\") | [timestamp] = {2026-01-01,2026-01-12} & ![timestamp] = \"2026-01-06\"",
            "([publication_year] = {2020,2025} & [language] = \"en\") | ([publication_year] = \"2019\" & ![title] = \"LLM\")"
        }
};

static const std::vector<std::vector<std::string>> dataset_fields = {
        // Dataset 1
        {"overall", "reviewtext"},

        // Dataset 2
        {"type", "created_at"},

        // Dataset 3
        {"page_title", "timestamp"},

        // Dataset 4
        {"publication_year", "language", "title"}
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

            for (size_t i = 0; i < bench_paths.size(); ++ i) {
                std::string bench_path = bench_paths[i];
                std::string full_data_path = data_path + dataset_size + bench_path;
                std::string full_index_path = index_path + file_type_to_str(file_type) + '/' + dataset_size + bench_path;

                for (auto enc_type : available_enc_types()) {
                    create_mode(full_index_path + '_' + enc_type_to_str(enc_type), full_data_path, dataset_fields[i], enc_type);
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

    state.SetItemsProcessed(state.iterations());
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

    state.SetItemsProcessed(state.iterations());
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

    state.SetItemsProcessed(state.iterations());
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

    state.SetItemsProcessed(state.iterations());
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

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::CSV, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::JSON, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 0>);
BENCHMARK_REGISTER_F(IndexSearch, simple_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::LOGFMT, 0>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::CSV, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::JSON, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 1>);
BENCHMARK_REGISTER_F(IndexSearch, union_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::LOGFMT, 1>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::CSV, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::JSON, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 2>);
BENCHMARK_REGISTER_F(IndexSearch, intersection_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::LOGFMT, 2>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::CSV, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::JSON, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 3>);
BENCHMARK_REGISTER_F(IndexSearch, combination_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::LOGFMT, 3>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::CSV, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::JSON, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT, 4>);
BENCHMARK_REGISTER_F(IndexSearch, complex_combination_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::LOGFMT, 4>);