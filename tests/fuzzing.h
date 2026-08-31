
#ifndef FUZZING_H
#define FUZZING_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <faker-cxx/faker.h>
#include <string>
#include <filesystem>
#include <fstream>
#include "../src/ingest.h"
#include "../src/helper.h"

void generateCSVData(const int rowCount, const std::vector<std::string> &fields, std::string &out);
void generateJSONData(const int rowCount, const std::vector<std::string> &fields, std::string &out);
void generateFuzzedDataFile(const uint8_t file_type, const uint32_t rowCount, const std::string &data_path, const std::vector<std::string> &fields);
std::vector<uint32_t> generateRandIntVec(const size_t size);
void generateFuzzedIndexFolder(const std::string &index_path);
std::vector<uint64_t> generate_rand_int_vec_dist(size_t size);

#endif