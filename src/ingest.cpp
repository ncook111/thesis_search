#include "ingest.h"
#include "coder.h"
#include "index.h"
#include "search.h"
#include "tokenizer.h"
#include "output.h"
#include "crc.h"
#include "metadata.h"
#include "segment.h"
#include "query/parser.h"
#include <cstdio>
#include <iostream>
#include <filesystem>
#include <boost/program_options.hpp>

int create_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint8_t enc_type, const uint32_t partition_size, const uint8_t num_threads) {
  if (std::filesystem::exists(index_path + "/.metadata")) {
    std::cout << "Index already exists\n";
    return 1;
  }

  if (data_path.empty()) {
    std::cout << "Data path missing, required when creating or updating an index. Usage: ./ingest --help\n";
    return 1;
  }

  try {
    create_index(fields, index_path, data_path, enc_type, partition_size, num_threads);
  } catch (const std::exception &e) {
    std::cout << "Error when creating index: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

int update_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields) {
  if (!std::filesystem::exists(index_path + "/.metadata")) {
    std::cout << "Not a valid existing index folder\n";
    return 1;
  }

  if (data_path.empty()) {
    std::cout << "Data path missing, required when creating or updating an index. Usage: ./ingest --help\n";
    return 1;
  }

  try {
    update_index(fields, index_path, data_path);
  } catch (const std::exception &e) {
    std::cout << "Error when updating index: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

int search_mode(const std::string &index_path, const std::string &data_path, const std::string &output_path, const std::string &query) {
  if (query.empty()) {
    std::cout << "No query values. Usage: ./ingest --help\n";
    return 1;
  }

  try {
    const Metadata metadata = Metadata::read(index_path);
    const FieldMap matches = parse_queries(query, metadata);
    if (data_path.size()) {
      output_entries(matches, metadata, data_path, output_path);
    } else {
      output_counts(matches, metadata, output_path);
    }
  } catch (const std::exception &e) {
    std::cout << "Error when searching index: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

int verify_mode(const std::string &index_path) {
  bool res = check_checksum_file(index_path);
  if (res) {
    std::cout << "Checksum Correct\n";
  } else {
    std::cout << "Checksum Incorrect\n";
  }

  return 0;
}

int parse_options(po::options_description &desc, po::variables_map &vm, const int argc, const char *argv[]) {
  try {
    desc.add_options()
    ("help", "produce help message")
    ("mode,m", po::value<std::string>(), "set operation mode [create/update/search/verify] or [c/r/s/v]")
    ("encoding,e", po::value<std::string>(), "set encoding variant [gvb/pef]")
    ("data-path,d", po::value<std::string>(), "input path, only required if mode=create/update, or if original output desired when mode=search")
    ("index-path,i", po::value<std::string>(), "index path")
    ("fields,f", po::value<std::vector<std::string>>()->multitoken(), "fields to index from the input data, optional")
    ("output-path,o", po::value<std::string>(), "output file, optional")
    ("query,q", po::value<std::string>(), "queries, only required if mode=search");

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);  
  } catch (std::exception& e) {
    std::cerr << e.what() << ": Usage: ./ingest --help\n";
    return 1;

  } catch(...) {
    std::cerr << "Exception of unknown type\n";
    return 1;
  }

  return 0;
}