#include <boost/crc.hpp>
#include <filesystem>
#include <fstream>
#include "helper.h"
#include "crc.h"

static const std::string CHECKSUM_FILE = ".checksum";

const uint32_t create_checksum(const std::string &file_name) {
    boost::crc_32_type crc;
    const MappedFile mf = open_mmap_r(file_name);
    const uint8_t *file = static_cast<uint8_t *>(mf.addr());
    const size_t file_size = mf.size();

    crc.process_bytes(file, file_size);

    return crc.checksum();
}

const bool verify_checksum(const std::string &file_name, const uint32_t checksum) {
    return checksum == create_checksum(file_name);
}

const std::vector<uint32_t> checksum_folder(std::string path) {
    // Get list of supported files to index
    std::vector<std::filesystem::path> files;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().filename() != CHECKSUM_FILE)
            files.push_back(entry.path());
    }

    std::sort(files.begin(), files.end());

    std::vector<uint32_t> checksums;
    for (auto &file : files) {
        checksums.push_back(create_checksum(file));
    }

    return checksums;
}

void create_checksum_file(const std::string &index_dir) {
    std::vector<uint32_t> checksums = checksum_folder(index_dir);
    write_checksum_to_file(index_dir, checksums);
}

const bool check_checksum_file(const std::string &index_dir) {
    std::vector<uint32_t> curr_checksums = checksum_folder(index_dir);
    std::vector<uint32_t> orig_checksums = read_checksum_from_file(index_dir);

    return curr_checksums == orig_checksums;
}

void write_checksum_to_file(const std::string &index_dir, const std::vector<uint32_t> &checksums) {
    std::ofstream file;
    file.open(index_dir + '/' + CHECKSUM_FILE, std::ios::binary);
    file.write(reinterpret_cast<const char *>(checksums.data()), checksums.size() * sizeof(uint32_t));
}

const std::vector<uint32_t> read_checksum_from_file(const std::string &index_dir) {
    std::ifstream file;
    file.open(index_dir + '/' + CHECKSUM_FILE, std::ios::binary);

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint32_t> checksums(size / sizeof(uint32_t));

    file.read(reinterpret_cast<char *>(checksums.data()), size);

    return checksums;
}