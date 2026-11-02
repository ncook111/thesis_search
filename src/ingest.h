#ifndef INGEST_H
#define INGEST_H

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int create_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint8_t enc_type, const uint32_t partition_size = 128 << 20, const uint32_t num_threads = std::max(1u, std::thread::hardware_concurrency()));
int update_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint32_t num_threads = std::max(1u, std::thread::hardware_concurrency()));
int search_mode(const std::string &index_path, const std::string &data_path, const std::string &output_path, const std::string &query, const uint32_t num_threads = std::max(1u, std::thread::hardware_concurrency()));
int verify_mode(const std::string &index_path);
int parse_options(po::options_description &desc, po::variables_map &vm, const int argc, const char *argv[]);

#endif