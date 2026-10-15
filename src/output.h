#ifndef OUTPUT_H
#define OUTPUT_H

#include <cstdint>
#include <string>
#include <map>
#include "search.h"
#include "metadata.h"

void output_entries(const FieldMap &matches, const Metadata &metadata,
                   const std::string &data_path, const std::string &output_path);
void output_counts(const FieldMap &matches, const Metadata &metadata, const std::string &output_path);
void entries_to_cout(const std::string &file_name, const std::vector<uint64_t> &entries, const char *data_file);
void entries_to_file(std::ofstream &output_file, const std::string &file_name, const std::vector<uint64_t> &entries, const char *data_file);
void counts_to_cout(const std::string &file_name, const size_t count);
void counts_to_file(std::ofstream &output_file, const std::string &file_name, const size_t count);

#endif