#include <cstdio>
#include <iostream>
#include "tokenizer.h"
#include "ingest.h"
#include "coder.h"
#include "index.h"

constexpr uint8_t ARG_MIN = 5; // Minimum arguments required for normal operation

int main(int argc, char *argv[]) {
    if (argc == 2 && std::string(argv[1]) == "help") {
        std::cout << "Usage: ./ingest [mode]" << std::endl;
        return 0;
    }

    if (argc < ARG_MIN) {
        std::cout << "Usage: ./ingest help" << std::endl;
        return 0;
    }

    const std::string act_mode = argv[1];
    const uint8_t enc_type = get_enc_type(argv[2]);
    const std::string data_path = argv[3];
    const std::string index_path = argv[4];

    // Create index mode
    if (act_mode == "-c") {
        if (std::filesystem::exists(index_path + "/file_mappings.fst")) {
            std::cout << "Index already exists" << std::endl;
            return 0;
        }

        create_index(index_path, data_path, enc_type);
    // Recreate index mode
    } else if (act_mode == "-r") {
        if (!std::filesystem::exists(index_path + "/file_mappings.fst")) {
            std::cout << "Not a valid existing index folder" << std::endl;
            return 0;
        }
        
        std::filesystem::remove_all(index_path);
        create_index(index_path, data_path, enc_type);
    // Index search mode
    } else if (act_mode == "-s") {
        if (argc == ARG_MIN) {
            std::cout << "Help: ingest help" << std::endl;
            return 0;
        }

        std::vector<std::string> queries;
        for (size_t i = ARG_MIN; i < argc; i++) {
            queries.push_back(argv[i]);
            std::cout << argv[i] << std::endl;
        }

        const EntryMap results = process_queries(queries, data_path, index_path, enc_type);
        //print_entries(results, index_path, data_path);
    }

    return 0;
}