#include "helper.h"
#include "../src/helper.h"

// Count should appear on second line
uint64_t get_count_from_file(const std::string &path) {
    std::ifstream in;
    in.open(path);

    if (!in.is_open()) {
        throw std::invalid_argument("File path not valid");
    }

    std::string line;
    std::getline(in, line);
    bool ret(std::getline(in, line));

    if (!ret) {
        throw std::runtime_error("Line could not be read");
    }

    size_t split = line.find(':');
    int value = std::stoi(line.substr(split + 1));

    return value;
}

std::string get_matching_query(const std::string &path, uint8_t file_type) {
    std::ifstream in;
    in.open(path);

    if (!in.is_open()) {
        throw std::invalid_argument("File path not valid");
    }

    switch (file_type) {
        case FileType::CSV:
            return get_matching_query_csv(in, path);
        case FileType::JSON:
            return get_matching_query_json(in, path);
        case FileType::LOGFMT:
            return get_matching_query_logfmt(in, path);
        default:
            throw std::invalid_argument("File type not valid");
    }
}

// 1982-04-08,82657517,272.12
// Looking to extract, for instance, the date here
std::string get_matching_query_csv(std::ifstream &in, const std::string &path) {
    // First line of file just header, need second line
    std::string line;
    std::getline(in, line);
    bool ret(std::getline(in, line));

    if (!ret) {
        throw std::runtime_error("Line could not be read");
    }

    size_t split = line.find(',');
    return line.substr(0, split);
}
// 1955-01-24","acc_nu
// {"dob":"1985-10-24","acc_num":"23862275","acc_bal":"2186.52"}
// Looking to extract, for instance, the date here
std::string get_matching_query_json(std::ifstream &in, const std::string &path) {
    std::string line;
    bool ret(std::getline(in, line));

    if (!ret) {
        throw std::runtime_error("Line could not be read");
    }

    size_t split_col = line.find(':');
    size_t split_com = line.find(',');
    return line.substr(split_col + 2, split_com - split_col - 3); // Extracts 1985-10-24, for instance
}

// dob=1928-12-05 acc_num=44484546 acc_bal=1592.69
// Looking to extract, for instance, the date here
std::string get_matching_query_logfmt(std::ifstream &in, const std::string &path) {
    std::string line;
    bool ret(std::getline(in, line));

    if (!ret) {
        throw std::runtime_error("Line could not be read");
    }

    size_t split_eq = line.find('=');
    size_t split_spce = line.find(' ');
    return line.substr(split_eq + 1, split_spce - split_eq - 1); // Extracts 1985-10-24, for instance
    // In the case of a multiword value, such as dob="1928 12 05", would only extract 1928, but this will still
    // be a matchable key for the index
}