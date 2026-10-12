#ifndef CRC_H
#define CRC_H

#include <string>
#include <vector>

const uint32_t create_checksum(const std::string &file_name);
const bool verify_checksum(const std::string &file_name, const uint32_t checksum);
const std::vector<uint32_t> checksum_folder(std::string path);
void create_checksum_file(const std::string &index_dir);
const bool check_checksum_file(const std::string &index_dir);
void write_checksum_to_file(const std::string &index_dir, const std::vector<uint32_t> &checksums);
const std::vector<uint32_t> read_checksum_from_file(const std::string &index_dir);

#endif