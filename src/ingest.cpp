#include "ingest.h"
#include "coder.h"
#include "index.h"
#include "search.h"
#include "tokenizer.h"
#include "output.h"
#include "query/parser.h"
#include <cstdio>
#include <iostream>

int create_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint8_t enc_type) {
  if (std::filesystem::exists(index_path + "/file_mappings.fst")) {
    std::cout << "Index already exists\n";
    return 1;
  }

  if (data_path.empty()) {
    std::cout << "Data path missing, required when creating or recreating an index. Usage: ./ingest --help\n";
    return 1;
  }

  try {
    create_index(fields, index_path, data_path, enc_type);
  } catch (const std::exception &e) {
    std::cout << "Error when creating index: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

int recreate_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint8_t enc_type) {
  if (!std::filesystem::exists(index_path + "/file_mappings.fst")) {
    std::cout << "Not a valid existing index folder\n";
    return 1;
  }

  if (data_path.empty()) {
    std::cout << "Data path missing, required when creating or recreating an index. Usage: ./ingest --help\n";
    return 1;
  }

  std::filesystem::remove_all(index_path);

  try {
    create_index(fields, index_path, data_path, enc_type);
  } catch (const std::exception &e) {
    std::cout << "Error when recreating index: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

int search_mode(const std::string &index_path, const std::string &data_path, const std::string &output_path, const std::vector<std::string> &fields, const std::vector<std::string> &queries, const uint8_t query_mode, const uint8_t enc_type) {
  if (query_mode == QueryMode::INVALID) {
    std::cout << "Query mode invalid. Usage: ./ingest --help\n";
    return 1;
  }

  if (queries.empty()) {
    std::cout << "No query values. Usage: ./ingest --help\n";
    return 1;
  }

  try {
    const FieldMap matches = parse_queries(queries.back(), index_path, enc_type);
    std::cout << matches.size() << std::endl;
    if (data_path.size()) {
      output_entries(matches, index_path, data_path, output_path, enc_type);
    } else {
      output_counts(matches, index_path, output_path);
    }
  } catch (const std::exception &e) {
    std::cout << "Error when searching index: " << e.what() << "\n";
    return 1;
  }

  return 0;
}