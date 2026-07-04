#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "helper.h"

using namespace boost::interprocess;

typedef std::unordered_map<size_t, std::vector<uint64_t>> FieldMap;
typedef std::unordered_map<size_t, std::unordered_map<uint64_t, uint64_t>> EntryMap;

uint8_t get_file_type(const std::string &type) {
    if (type == ".csv") return 0;
    if (type == ".json") return 0;
    if (type == ".logfmt") return 0;

    throw std::invalid_argument("File type not supported");
}

MappedFile open_mmap_r(std::string file_path) {
    file_mapping file_mapping(file_path.data(), read_only);
    mapped_region file_region(file_mapping, read_only);

    void * file_addr = file_region.get_address();
    std::size_t size = file_region.get_size();

    return MappedFile{std::move(file_mapping), std::move(file_region)};
}

MappedFile open_mmap_rw(std::string file_path) {
    
    file_mapping file_mapping(file_path.data(), read_write);
    mapped_region file_region(file_mapping, read_write);

    void * file_addr = file_region.get_address();
    std::size_t size = file_region.get_size();

    return MappedFile{std::move(file_mapping), std::move(file_region)};
}