#include "ingest.h"
#include "coder.h"
#include "index.h"
#include "search.h"
#include "tokenizer.h"
#include "output.h"
#include "query/parser.h"
#include "indexer.h"
#include <cstdio>
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, const char *argv[]) {
  po::options_description desc("Allowed options");
  po::variables_map vm;

  if (parse_options(desc, vm, argc, argv))
    return 1;

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }


  std::string act_mode;
  if (vm.count("mode")) {
    act_mode = vm["mode"].as<std::string>();
  } else {
    std::cout << "Mode argument missing. Usage: ./ingest --help\n";
    return 1;
  }

  uint8_t enc_type = EncType::INVALID;
  if (vm.count("encoding")) {
    try {
      enc_type = str_to_enc_type(vm["encoding"].as<std::string>());
    } catch (std::exception& e) {
      std::cout << "Encoding argument invalid. Usage: ./ingest --help\n";
      return 1;
    }
  } else if (act_mode != "v" && act_mode !=  "verify" && act_mode != "u" && act_mode !=  "update"){
    std::cout << "Encoding argument missing. Usage: ./ingest --help\n";
    return 1;
  }

  uint32_t partition_size;
  if (vm.count("partition")) {
    partition_size = vm["partition"].as<uint32_t>() << 20; // Convert into MB
  } else {
    partition_size = 128 << 20; // Default
  }

  uint32_t num_threads;
  if (vm.count("threads")) {
    num_threads = vm["threads"].as<uint32_t>();
  } else {
    num_threads = std::thread::hardware_concurrency(); // Default
  }

  std::string data_path;
  if (vm.count("data-path"))
    data_path = vm["data-path"].as<std::string>();

  std::string index_path;
  if (vm.count("index-path")) {
    index_path = vm["index-path"].as<std::string>();
  } else {
    std::cout << "Index path missing. Usage: ./ingest --help\n";
    return 1;
  }

  std::vector<std::string> fields;
  if (vm.count("fields"))
    fields = vm["fields"].as<std::vector<std::string>>();

  std::string output_path;
  if (vm.count("output-path"))
    output_path = vm["output-path"].as<std::string>();

  std::string queries;
  if (vm.count("query")) {
    queries = vm["query"].as<std::string>();
    std::cout << queries << std::endl;
  }

  if (act_mode == "c" || act_mode == "create") { // Index create mode
    return create_mode(index_path, data_path, fields, enc_type, partition_size, num_threads);
  } else if (act_mode == "u" || act_mode ==  "update") { // Index update mode
    return update_mode(index_path, data_path, fields);
  } else if (act_mode == "s" || act_mode ==  "search") { // Index search mode
    return search_mode(index_path, data_path, output_path, queries);
  } else if (act_mode == "v" || act_mode ==  "verify") { // Index verify
    return verify_mode(index_path);
  } else {
    std::cout << "Invalid mode. Usage: ./ingest --help\n";
    return 1;
  }
}
