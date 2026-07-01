#include <cstdio>
#include <iostream>
#include "tokenizer.h"
#include "ingest.h"
#include "coder.h"
#include "index.h"


int main(int argc, char *argv[]) {
    const std::string index_path = argv[1];
    const std::string data_path = argv[2];
    const std::string query_1 = argv[3];
    const std::string query_2 = argv[4];
    const std::string query_3 = argv[5];

    create_index(index_path, data_path);

    // const std::unordered_map<size_t, std::vector<uint64_t>> offsets_1 = search_index(query_1);
    // const std::unordered_map<size_t, std::vector<uint64_t>> offsets_2 = search_index(query_2);
    // const std::unordered_map<size_t, std::vector<uint64_t>> offsets_3 = search_index(query_3);

    // fst::StdConstFst *fst = nullptr;
    // read_from_disk(fst, "index/file_mappings.fst");
    // std::set<std::string> names = get_strings_from_fst<std::string>(*fst);

    // std::vector<std::string> names_vec;

    // for (auto &i : names) {
    //     names_vec.push_back(i);
    // }

    // std::cout << std::endl << "-----OUTPUT-----" << std::endl << std::endl;

    // const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> lines_1 = get_entries_offsets(offsets_1, names_vec, data_path, index_path);
    // const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> lines_2 = get_entries_offsets(offsets_2, names_vec, data_path, index_path);
    // const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> lines_3 = get_entries_offsets(offsets_3, names_vec, data_path, index_path);

    // const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> tmp = get_entries_offsets_intersection(lines_1, lines_2);
    // const std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> intersection_lines = get_entries_offsets_intersection(tmp, lines_3);

    // for (auto &i : tmp) {
    //     std::cout << "File: " << names_vec[i.first] << std::endl;
    //     const uint64_t file_id = i.first;
    //     const std::string file_path = data_path + '/' + names_vec[file_id];

    //     std::ifstream file;
    //     file.open(file_path, std::ios::binary);

    //     if (!file.is_open()) {
    //         throw std::runtime_error("Failed to open file: " + file_path);
    //     }
    //     for (auto &line : i.second) {
    //         std::cout << "Line: " << get_entry_str(std::make_pair(line.first, line.second), file) << std::endl;
    //     }
    // }
}

