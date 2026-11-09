#include <benchmark/benchmark.h>
#include <filesystem>
#include <mutex>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <set>
#include "cache_flush.h"
#include "../src/ingest.h"
#include "../src/helper.h"

#if defined(__linux__)
#include <malloc.h>
#endif

constexpr size_t cache_size = 512000 * 1024;

static std::string csv_data_path = BENCH_DATA_DIR "/all_input_data/csv/";
static std::string json_data_path = BENCH_DATA_DIR "/all_input_data/json/";
static std::string logfmt_data_path = BENCH_DATA_DIR "/all_input_data/logfmt/";
static std::string index_path = BENCH_DATA_DIR "/temp_indexes/index_search/";

#if defined(__linux__)
static void reset_rss_hwm() { std::ofstream("/proc/self/clear_refs") << "5"; }

static size_t read_rss_kb(const char* key) {
    std::ifstream f("/proc/self/status");
    for (std::string line; std::getline(f, line);)
        if (line.rfind(key, 0) == 0) return std::stoul(line.substr(std::strlen(key)));
    return 0;
}
#endif

static benchmark::Counter kb_counter(double kb) {
    return benchmark::Counter(kb * 1024.0, benchmark::Counter::kDefaults,
                              benchmark::Counter::kIs1024);
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
        ->ArgNames({"size", "dataset", "type", "enc", "partition_size", "num_threads", "query_type"})
        ->ArgsProduct({
            {FileSize},
            {0, 1, 2, 3},
            {FileType},
            available_enc_types(),
            {8, 16, 32, 64, 128, 256, 512, 1024},
            {1, 2, 4, 6, 8, 10, 12, 14, 16, 32}
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
            "[reviewText] = \"book\"", 
            "[type] = \"IssueCommentEvent\"", 
            "[page_title] = \"germany\"", 
            "[publication_year] = \"2026\""
        },

        // union_search
        {
            "[reviewText] = \"book\" | [reviewText] = \"television\"",
            "[type] = \"IssueCommentEvent\" | [type] = \"PullRequestReviewCommentEvent\"",
            "[page_title] = \"germany\" | [page_title] = \"england\"",
            "[publication_year] = \"2026\" | [publication_year] = \"2025\""
        },

        // intersection_search
        {
            "[reviewText] = \"book\" & [reviewText] = \"paperback\"",
            "[type] = \"IssueCommentEvent\" & [id] = \"5562772598\"",
            "[page_title] = \"pdf\" & [page_title] = \"germany\"",
            "[publication_year] = \"2026\" & [language] = \"en\""
        },

        // combination_search
        {
            "([overall] = \"5.0\" & [reviewText] = \"book\") & [reviewText] = \"paperback\"",
            "([type] = \"IssueCommentEvent\" | [type] = \"PullRequestReviewCommentEvent\") & [id] = \"5562772598\"",
            "([page_title] = \"germany\" | [page_title] = \"england\") | [page_title] = \"pdf\"",
            "([publication_year] = \"2026\" | [publication_year] = \"2025\") & [language] = \"en\""
        },

        // complex combination_search
        {
            "([overall] = \"5.0\" & [reviewText] = \"paperback\") | [overall] = {3.0,4.0}",
            "([type] = \"IssueCommentEvent\" | [type] = \"PullRequestReviewCommentEvent\") & [id] = {1162772598,5862772598}",
            "([page_title] = \"germany\" | [page_title] = \"england\") & ([page_title] = \"pdf\" | [page_title] = \".html\")",
            "([publication_year] = {2020,2025} & [language] = \"en\") | [publication_year] = \"2019\""
        }
};

static const std::vector<std::vector<std::string>> dataset_fields = {
        // Dataset 1
        {"overall", "reviewText"},

        // Dataset 2
        {"type", "id"},

        // Dataset 3
        {"page_title", "timestamp"},

        // Dataset 4
        {"publication_year", "language"}
}; 

class IndexSearch : public benchmark::Fixture {
public:
    int dataset_number;
    std::string bench_path;
    uint8_t enc_type;
    size_t partition_size;
    size_t num_threads;
    void SetUp(::benchmark::State& state) {
        std::string dataset_size = file_size_to_str(state.range(0));
        dataset_number = state.range(1);
        uint8_t file_type = state.range(2);
        enc_type = state.range(3);
        partition_size = state.range(4);
        num_threads = state.range(5);
        state.SetLabel(enc_type_to_str(enc_type) + ',' + dataset_size + ',' + file_type_to_str(file_type) + ',' +
                       std::to_string(dataset_number) + ',' + std::to_string(partition_size) + ',' + std::to_string(num_threads));

        // partition_size part of the index location
        bench_path = file_type_to_str(file_type) + '/' + dataset_size + "_data/partition_" + std::to_string(partition_size) +
                     "/data_" + std::to_string(dataset_number + 1) + '_' + enc_type_to_str(enc_type);

        //std::call_once(wipe_flag, [] { std::filesystem::remove_all(index_path); });

        std::string data_file = data_dir_for(file_type) + dataset_size + "_data/data_" + std::to_string(dataset_number + 1);
        ensure_index(bench_path, data_file, dataset_fields[dataset_number], enc_type, partition_size << 20);
    
    }

private:
    static std::string data_dir_for(uint8_t file_type) {
        switch (file_type) {
            case FileType::CSV:  return csv_data_path;
            case FileType::JSON: return json_data_path;
            default:             return logfmt_data_path;
        }
    }

    // Builds each distinct index (size, type, dataset, enc, partition_size) exactly once.
    static void ensure_index(const std::string& rel_path, const std::string& data_file,
                             const std::vector<std::string>& fields, uint8_t enc, size_t part) {
        static std::set<std::string> built;
        if (built.count(rel_path)) return;

        std::string full = index_path + rel_path;
        std::filesystem::create_directories(std::filesystem::path(full).parent_path());
        create_mode(full, data_file, fields, enc, part);
        built.insert(rel_path);   // only mark as built after create_mode succeeds
    }

    static std::once_flag wipe_flag;
};

std::once_flag IndexSearch::wipe_flag;

BENCHMARK_DEFINE_F(IndexSearch, simple_search)(benchmark::State& state) {
    std::string query = queries[0][dataset_number];
    std::cout << query << "\n";
    #if defined(__linux__)
    double rss_before = 0;
    #endif
    for (auto _ : state) {
        state.PauseTiming();
        #if defined(__linux__)
        malloc_trim(0);  
        reset_rss_hwm();
        rss_before = read_rss_kb("VmRSS:");
        #endif
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, num_threads);
        benchmark::ClobberMemory();
    }

    #if defined(__linux__)
    const double peak_kb = read_rss_kb("VmHWM:");
    malloc_trim(0);
    const double rss_trimmed = read_rss_kb("VmRSS:");

    state.counters["peak_rss"]       = kb_counter(peak_kb);
    state.counters["peak_delta"]     = kb_counter(peak_kb - rss_before);
    state.counters["residual"]       = kb_counter(rss_trimmed - rss_before);
    #endif

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(IndexSearch, union_search)(benchmark::State& state) {
    std::string query = queries[1][dataset_number];
    std::cout << query << "\n";
    #if defined(__linux__)
    double rss_before = 0;
    #endif
    for (auto _ : state) {
        state.PauseTiming();
        #if defined(__linux__)
        malloc_trim(0);  
        reset_rss_hwm();
        rss_before = read_rss_kb("VmRSS:");
        #endif
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, num_threads);
        benchmark::ClobberMemory();
    }

    #if defined(__linux__)
    const double peak_kb = read_rss_kb("VmHWM:");
    malloc_trim(0);
    const double rss_trimmed = read_rss_kb("VmRSS:");

    state.counters["peak_rss"]       = kb_counter(peak_kb);
    state.counters["peak_delta"]     = kb_counter(peak_kb - rss_before);
    state.counters["residual"]       = kb_counter(rss_trimmed - rss_before);
    #endif

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(IndexSearch, intersection_search)(benchmark::State& state) {
    std::string query = queries[2][dataset_number];
    std::cout << query << "\n";
    #if defined(__linux__)
    double rss_before = 0;
    #endif
    for (auto _ : state) {
        state.PauseTiming();
        #if defined(__linux__)
        malloc_trim(0);  
        reset_rss_hwm();
        rss_before = read_rss_kb("VmRSS:");
        #endif
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, num_threads);
        benchmark::ClobberMemory();
    }

    #if defined(__linux__)
    const double peak_kb = read_rss_kb("VmHWM:");
    malloc_trim(0);
    const double rss_trimmed = read_rss_kb("VmRSS:");

    state.counters["peak_rss"]       = kb_counter(peak_kb);
    state.counters["peak_delta"]     = kb_counter(peak_kb - rss_before);
    state.counters["residual"]       = kb_counter(rss_trimmed - rss_before);
    #endif

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(IndexSearch, combination_search)(benchmark::State& state) {
    std::string query = queries[3][dataset_number];
    std::cout << query << "\n";
    #if defined(__linux__)
    double rss_before = 0;
    #endif
    for (auto _ : state) {
        state.PauseTiming();
        #if defined(__linux__)
        malloc_trim(0);  
        reset_rss_hwm();
        rss_before = read_rss_kb("VmRSS:");
        #endif
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, num_threads);
        benchmark::ClobberMemory();
    }

    #if defined(__linux__)
    const double peak_kb = read_rss_kb("VmHWM:");
    malloc_trim(0);
    const double rss_trimmed = read_rss_kb("VmRSS:");

    state.counters["peak_rss"]       = kb_counter(peak_kb);
    state.counters["peak_delta"]     = kb_counter(peak_kb - rss_before);
    state.counters["residual"]       = kb_counter(rss_trimmed - rss_before);
    #endif

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(IndexSearch, complex_combination_search)(benchmark::State& state) {
    std::string query = queries[4][dataset_number];
    std::cout << query << "\n";
    #if defined(__linux__)
    double rss_before = 0;
    #endif
    for (auto _ : state) {
        state.PauseTiming();
        #if defined(__linux__)
        malloc_trim(0);  
        reset_rss_hwm();
        rss_before = read_rss_kb("VmRSS:");
        #endif
        flush_cache(cache_size);
        state.ResumeTiming();
        search_mode(index_path + bench_path, "", {}, query, num_threads);
        benchmark::ClobberMemory();
    }

    #if defined(__linux__)
    const double peak_kb = read_rss_kb("VmHWM:");
    malloc_trim(0);
    const double rss_trimmed = read_rss_kb("VmRSS:");

    state.counters["peak_rss"]       = kb_counter(peak_kb);
    state.counters["peak_delta"]     = kb_counter(peak_kb - rss_before);
    state.counters["residual"]       = kb_counter(rss_trimmed - rss_before);
    #endif

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