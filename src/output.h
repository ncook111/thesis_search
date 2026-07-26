#ifndef OUTPUT_H
#define OUTPUT_H

#include <cstdint>
#include <string>
#include <map>
#include "search.h"

void output_entries(const FieldMap &matches, const std::string &index_path,
                   const std::string &data_path, const std::string &output_path, const uint8_t enc_type);
void output_counts(const FieldMap &matches, const std::string &index_path, const std::string &output_path);
void entries_to_cout(const std::string &file_name, const std::__1::vector<uint64_t> &entries, const char *data_file);
void entries_to_file(const std::string &output_path, const std::string &file_name, const std::__1::vector<uint64_t> &entries, const char *data_file);
void counts_to_cout(const std::string &file_name, const size_t count);
void counts_to_file(const std::string &output_path, const std::string &file_name, const size_t count);

#endif