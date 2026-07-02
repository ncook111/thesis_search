#include <cstdio>
#include <iostream>
#include "tokenizer.h"
#include "ingest.h"
#include "coder.h"
#include "index.h"


constexpr uint8_t ARG_MIN = 4; // Minimum arguments required for normal operation

int main(int argc, char *argv[]) {
    if (argc == 2 && std::string(argv[1]) == "help") {
        std::cout << "Usage: ./ingest [mode]" << std::endl;
        return 0;
    }

    if (argc < ARG_MIN) {
        std::cout << "Usage: ./ingest help" << std::endl;
        return 0;
    }

    const std::string mode = argv[1];
    const std::string data_path = argv[2];
    const std::string index_path = argv[3];


    // Create index mode
    if (mode == "-c") {
        create_index(index_path, data_path);
    // Recreate index mode
    } else if (mode == "-r") {
        create_index(index_path, data_path);
    // Index search mode
    } else if (mode == "-s") {

        if (argc == ARG_MIN) {
            std::cout << "Help: ingest help" << std::endl;
            return 0;
        }

        std::vector<std::string> queries;
        for (size_t i = ARG_MIN; i < argc; i++) {
            queries.push_back(argv[i]);
            std::cout << argv[i];
        }

        process_queries(queries, data_path, index_path);
    }

    return 0;
}