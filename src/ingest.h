#ifndef INGEST_H
#define INGEST_H

#include <cstddef>
#include <cstdint>
#include <iostream>

int main(int argc, char *argv[]);
int create_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint8_t enc_type);
int recreate_mode(const std::string &index_path, const std::string &data_path, const std::vector<std::string> &fields, const uint8_t enc_type);
int search_mode(const std::string &index_path, const std::string &data_path, const std::string &output_path, const std::vector<std::string> &fields, const std::vector<std::string> &queries, const uint8_t query_mode, const uint8_t enc_type);

#endif