#ifndef INGEST_H
#define INGEST_H

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int create_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint8_t enc_type);
int recreate_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint8_t enc_type);
int search_mode(const std::string &index_path, const std::string &data_path, const std::string &output_path, const std::string &query);
int verify_mode(const std::string &index_path);
int parse_options(po::options_description &desc, po::variables_map &vm, const int argc, const char *argv[]);

#endif