#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <faker-cxx/faker.h>
#include <string>
#include <filesystem>
#include <fstream>
#include "../src/ingest.h"
#include "../src/helper.h"

void generateCSVData(const int rowCount, const std::vector<std::string> &fields, std::string &out) {
    for (auto &field : fields) {
        out += field + ',';
    }


    out.back() = '\n'; // Remove trailing comma and replace with nl

    for (int i = 0; i < rowCount; ++i) {
        out += faker::date::birthdateByYear().substr(0, 10);
        out += ',';
        out += faker::finance::accountNumber();
        out += ',';
        out += faker::finance::amount(0, 5000);
        out += '\n';
    }
}

void generateJSONData(const int rowCount, const std::vector<std::string> &fields, std::string &out) {
    for (int i = 0; i < rowCount; ++i) {
        out += '{';
        out += '"' + fields[0] + "\":" + '"' + faker::date::birthdateByYear().substr(0, 10) + '"' + ',';
        out += '"' + fields[1] + "\":" + '"' + faker::finance::accountNumber() + '"' + ',';
        out += '"' + fields[2] + "\":" + '"' + faker::finance::amount(0, 5000) + '"';
        out += "}\n";
    }
}

void generateFuzzedDataFile(const uint8_t file_type, const uint32_t rowCount, const std::string &data_path, const std::vector<std::string> &fields) {
    std::filesystem::create_directory(data_path);

    std::ofstream file;
    
    std::string out;
    if (file_type == FileType::CSV) {
        file.open(data_path + "/fuzzed.csv", std::ios::binary);
        generateCSVData(rowCount, fields, out);
    } else if (file_type == FileType::JSON) {
        file.open(data_path + "/fuzzed.json", std::ios::binary);
        generateJSONData(rowCount, fields, out);
    } else {
        throw std::invalid_argument("FileType not supported/invalid");
    }

    file.write(out.data(), out.size());
    file.close();
}

std::vector<uint32_t> generateRandIntVec(const size_t size) {
    std::mt19937 eng(42);
    std::uniform_int_distribution<uint32_t> distr_32;

    std::vector<uint32_t> rand_ints;
    rand_ints.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        rand_ints.push_back(distr_32(eng));
    }

    return rand_ints;
}

void generateFuzzedIndexFile(const std::string &file_path) {
    std::vector<uint32_t> rand_ints = generateRandIntVec(1000000);
    
    std::ofstream file;
    file.open(file_path, std::ios::binary);

    file.write(reinterpret_cast<const char *>(rand_ints.data()), rand_ints.size() * sizeof(uint32_t));
    file.close();
}

void generateFuzzedIndexFolder(const std::string &index_path) {
    std::filesystem::create_directory(index_path);

    generateFuzzedIndexFile(index_path + "/fuzzed_1.bin");
    generateFuzzedIndexFile(index_path + "/fuzzed_2.bin");
    generateFuzzedIndexFile(index_path + "/fuzzed_3.bin");
    generateFuzzedIndexFile(index_path + "/fuzzed_4.bin");
}