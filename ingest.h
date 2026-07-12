#ifndef INGEST_H
#define INGEST_H

#include <iostream>
#include <cstdint>
#include <cstddef>

int main(int argc, char *argv[]);
int create_mode(const std::string &index_path, const std::string &data_path, const uint8_t enc_type);
int recreate_mode(const std::string &index_path, const std::string &data_path, const uint8_t enc_type);
int search_mode(const std::string &index_path, const std::string &data_path, const uint8_t enc_type, const int argc, char *argv[]);

#endif