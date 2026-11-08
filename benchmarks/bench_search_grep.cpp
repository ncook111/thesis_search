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

constexpr size_t cache_size = 2048 * 1024;

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

template <uint8_t FileSize, uint8_t FileType> static void StandardBenchArgs(benchmark::internal::Benchmark* args) {
    args->Name(std::string("IndexSearchBaseline/") + file_size_to_str(FileSize) + '/' + file_type_to_str(FileType))
        ->ArgNames({"size", "dataset", "type", "enc"})
        ->ArgsProduct({
            {FileSize},
            {0, 1, 2, 3},
            {FileType},
            available_enc_types()
        })
        ->Iterations(1)
        ->UseRealTime()
        ->Unit(benchmark::kMillisecond)
        ->ComputeStatistics("p95", [](const std::vector<double>& v) { return get_percentile(v, 0.95); })
        ->ComputeStatistics("max", [](const std::vector<double>& v) { return *std::max_element(v.begin(), v.end()); });
}

static const std::vector<std::string> queries = {
    "[*] = \"book\"",
    "[*] = \"IssueCommentEvent\"",
    "[*] = \"germany\"",
    "[*] = \"2026\""
};

class IndexSearchBaseline : public benchmark::Fixture {
public:
    int dataset_number;
    std::string bench_path;
    uint8_t enc_type;
    void SetUp(::benchmark::State& state) {
        std::string dataset_size = file_size_to_str(state.range(0));
        dataset_number = state.range(1);
        uint8_t file_type = state.range(2);
        enc_type = state.range(3);
        state.SetLabel(enc_type_to_str(enc_type) + ',' + dataset_size + ',' + file_type_to_str(file_type) + ',' +
                       std::to_string(dataset_number));

        bench_path = file_type_to_str(file_type) + '/' + dataset_size + "_data/" +
                     "data_" + std::to_string(dataset_number + 1) + '_' + enc_type_to_str(enc_type);

        std::call_once(wipe_flag, [] { std::filesystem::remove_all(index_path); });

        std::string data_file = data_dir_for(file_type) + dataset_size + "_data/data_" + std::to_string(dataset_number + 1);
        ensure_index(bench_path, data_file, {}, enc_type);
    
    }

private:
    static std::string data_dir_for(uint8_t file_type) {
        switch (file_type) {
            case FileType::CSV:  return csv_data_path;
            case FileType::JSON: return json_data_path;
            default:             return logfmt_data_path;
        }
    }

    // Builds each distinct index (size, type, dataset, enc) exactly once.
    static void ensure_index(const std::string& rel_path, const std::string& data_file,
                             const std::vector<std::string>& fields, uint8_t enc) {
        static std::set<std::string> built;
        if (built.count(rel_path)) return;

        std::string full = index_path + rel_path;
        std::filesystem::create_directories(std::filesystem::path(full).parent_path());
        create_mode(full, data_file, fields, enc);
        built.insert(rel_path);   // only mark as built after create_mode succeeds
    }

    static std::once_flag wipe_flag;
};

std::once_flag IndexSearchBaseline::wipe_flag;

BENCHMARK_DEFINE_F(IndexSearchBaseline, simple_search)(benchmark::State& state) {
    const std::string& query = queries[dataset_number];
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
        search_mode(index_path + bench_path, "", {}, query, 1);
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

BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::CSV>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::CSV>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::CSV>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::CSV>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::JSON>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::JSON>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::JSON>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::JSON>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::SMALL, FileType::LOGFMT>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::MEDIUM, FileType::LOGFMT>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::LARGE, FileType::LOGFMT>);
BENCHMARK_REGISTER_F(IndexSearchBaseline, simple_search)->Apply(StandardBenchArgs<FileSize::XLARGE, FileType::LOGFMT>);