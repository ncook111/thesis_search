#ifndef HLPER_H
#define HLPER_H

#include <string>
#include <cstdint>
#include <fstream>

uint64_t get_count_from_file(const std::string &path);
std::string get_matching_query(const std::string &path , const uint8_t file_type);
std::string get_matching_query_csv(std::ifstream &in, const std::string &path);
std::string get_matching_query_json(std::ifstream &in, const std::string &path);
std::string get_matching_query_logfmt(std::ifstream &in, const std::string &path);

#endif