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
        if (std::filesystem::exists(index_path + "/file_mappings.fst")) {
            std::cout << "Index already exists" << std::endl;
            return 0;
        }
        create_index(index_path, data_path);
    // Recreate index mode
    } else if (mode == "-r") {
        if (!std::filesystem::exists(index_path + "/file_mappings.fst")) {
            std::cout << "Not a valid existing index folder" << std::endl;
            return 0;
        }
        
        std::filesystem::remove_all(index_path);
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

        const EntryMap results = process_queries(queries, data_path, index_path);
        print_entries(results, index_path, data_path);
    }

    // std::vector<uint64_t> in;

    // for (int i = 0; i < 100000000; i+=4) {
    //     in.push_back(i);
    // }

    // uint64_t universe = in.back() + 1;
    // uint64_t n = in.size();
    // ds2i::global_parameters params;
    // succinct::bit_vector_builder bvb;

    // ds2i::partitioned_sequence<>::write(bvb, in.begin(), universe, n, params); 

    // succinct::bit_vector bv(&bvb);

    // std::cout << "Input Size: " << in.size() * 8 << std::endl;
    // std::cout << "Output Size: " << bv.data().size() * sizeof(uint64_t) << std::endl;

    // ds2i::partitioned_sequence<>::enumerator e(bv, 0, universe, n, params);

    // std::vector<uint64_t> decoded;
    // decoded.reserve(n);
    // for (uint64_t i = 0; i < n; ++i) {
    //     decoded.push_back(e.move(i).second);
    // }

    // std::cout << "Correct: " << (decoded == in) << std::endl;

    // std::vector<uint8_t> gvb_encoded = gvb_encode_uint64_t(in);
    // std::vector<uint64_t> gvb_decoded = gvb_decode_uint64_t(gvb_encoded);

    // std::cout << "Input Size: " << in.size() * 8 << std::endl;
    // std::cout << "Output Size: " << (float)(gvb_encoded.size()) / (in.size() * 8) << std::endl;


    return 0;
}