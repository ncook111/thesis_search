#include "ingest.h"
#include "coder.h"
#include "index.h"
#include "search.h"
#include "tokenizer.h"
#include "output.h"
#include "query/parser.h"
#include <cstdio>
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  // Declare the supported options.
  po::options_description desc("Allowed options");
  po::variables_map vm;

  try {
    desc.add_options()
    ("help", "produce help message")
    ("mode", po::value<std::string>(), "set operation mode [create/recreate/search] or [c/r/s]")
    ("encoding", po::value<std::string>(), "set encoding variant [gvb/pef]")
    ("data-path,d", po::value<std::string>(), "input path, only required if mode=create/recreate, or if original output desired when mode=search")
    ("index-path,i", po::value<std::string>(), "index path")
    ("fields,f", po::value<std::vector<std::string>>()->multitoken(), "fields to index from the input data, optional")
    ("output-path,o", po::value<std::string>(), "output file, optional")
    ("query-mode", po::value<std::string>(), "query mode, only required if mode=search")
    ("queries,q", po::value<std::vector<std::string>>()->multitoken(), "queries, only required if mode=search");

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);  
  } catch (std::exception& e) {
    std::cerr << e.what() << ": Usage: ./ingest --help\n";
    return 1;

  } catch(...) {
    std::cerr << "Exception of unknown type\n";
    return 1;
  }



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
      enc_type = get_enc_type(vm["encoding"].as<std::string>());
    } catch (std::exception& e) {
      std::cout << "Encoding argument invalid. Usage: ./ingest --help\n";
      return 1;
    }
  } else {
    std::cout << "Encoding argument missing. Usage: ./ingest --help\n";
    return 1;
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


  uint8_t query_mode = QueryMode::INVALID;
  if (vm.count("query-mode")) {
    try {
      query_mode = get_query_mode(vm["query-mode"].as<std::string>());
    } catch (std::exception& e) {
      std::cout << "Query mode invalid. Usage: ./ingest --help\n";
      return 1;
    }
  }

  std::vector<std::string> queries;
  if (vm.count("queries"))
    queries = vm["queries"].as<std::vector<std::string>>();

  if (act_mode == "c" || act_mode == "create") { // Create index mode
    return create_mode(index_path, data_path, fields, enc_type);
  } else if (act_mode == "r" || act_mode ==  "recreate") { // Recreate index mode
    return recreate_mode(index_path, data_path, fields, enc_type);
  } else if (act_mode == "s" || act_mode ==  "search") { // Index search mode
    
    return search_mode(index_path, data_path, output_path, fields, queries, query_mode, enc_type);
  } else {
    std::cout << "Invalid mode. Usage: ./ingest --help\n";
    return 1;
  }
}