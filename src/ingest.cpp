#include "ingest.h"
#include "coder.h"
#include "index.h"
#include "search.h"
#include "tokenizer.h"
#include <cstdio>
#include <iostream>

// Minimum arguments required for normal operation
constexpr uint8_t ARG_MIN = 5;

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

  if (act_mode == "-c") { // Create index mode
    return create_mode(index_path, data_path, enc_type, argc, argv);
  } else if (act_mode == "-r") { // Recreate index mode
    return recreate_mode(index_path, data_path, enc_type, argc, argv);
  } else if (act_mode == "-s") { // Index search mode
    return search_mode(index_path, data_path, enc_type, argc, argv);
  } else {
    std::cout << "Invalid mode. Help: ingest help" << std::endl;
    return 1;
  }
}

int create_mode(const std::string &index_path, const std::string &data_path,
                const uint8_t enc_type, const int argc, char *argv[]) {
  if (std::filesystem::exists(index_path + "/file_mappings.fst")) {
    std::cout << "Index already exists" << std::endl;
    return 1;
  }

  std::vector<std::string> fields;
  for (size_t i = ARG_MIN; i < static_cast<uint64_t>(argc); i++) {
    fields.push_back(argv[i]);
  }

  try {
    create_index(fields, index_path, data_path, enc_type);
  } catch (const std::exception &e) {
    std::cout << "Error when creating index: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

int recreate_mode(const std::string &index_path, const std::string &data_path,
                  const uint8_t enc_type, const int argc, char *argv[]) {
  if (!std::filesystem::exists(index_path + "/file_mappings.fst")) {
    std::cout << "Not a valid existing index folder" << std::endl;
    return 1;
  }

  std::filesystem::remove_all(index_path);

  std::vector<std::string> fields;
  for (size_t i = ARG_MIN; i < static_cast<uint64_t>(argc); i++) {
    fields.push_back(argv[i]);
  }

  try {
    create_index(fields, index_path, data_path, enc_type);
  } catch (const std::exception &e) {
    std::cout << "Error when recreating index: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

int search_mode(const std::string &index_path, const std::string &data_path,
                const uint8_t enc_type, const int argc, char *argv[]) {
  if (argc == ARG_MIN + 1) {
    std::cout << "Help: ingest help" << std::endl;
    return 1;
  }

  uint8_t query_mode = get_query_mode(argv[ARG_MIN]);
  std::cout << argv[ARG_MIN] << std::endl;

  std::vector<std::string> queries;
  for (size_t i = ARG_MIN + 1; i < static_cast<uint64_t>(argc); i++) {
    queries.push_back(argv[i]);
  }

  try {
    const EntryMap results =
    process_queries(queries, data_path, index_path, enc_type, query_mode);
    //print_entries(results, index_path, data_path);
    std::cout << results.at(0).size() / 2 << std::endl;
  } catch (const std::exception &e) {
    std::cout << "Error when searching index: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}