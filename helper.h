#ifndef HELPER_H
#define HELPER_H

#include <vector>
#include <map>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "custom_types.h"

using namespace boost::interprocess;

struct MappedFile {
    file_mapping mapping;
    mapped_region region;

    void* addr() const { return region.get_address(); }
    std::size_t size() const { return region.get_size(); }
};

namespace FileType {
    enum Value : uint8_t {
        CSV = 0,
        JSON = 1,
        LOGFMT = 2,
        INVALID = 3
    };
}

namespace EncType {
    enum Value : uint8_t {
        GVB = 0,
        PEF = 1,
    };
}

namespace QueryMode {
    enum Value : uint8_t {
        UNION = 0,
        INTER = 1,
    };
}

const bool pef_or_fst(const uint8_t type);
const bool valid_extension(const std::string &ext);
const uint8_t get_file_type(const std::string &type);
const uint8_t get_enc_type(const std::string &type);
const MappedFile open_mmap_r(const std::string &file_path);
const MappedFile open_mmap_rw(const std::string &file_path);
const std::string token_type_to_str(const uint8_t &type);
const uint8_t str_to_token_type(const std::string &type_str);
const std::vector<uint64_t> remove_dups(const std::vector<uint64_t> &vec);
const uint8_t get_query_mode(const std::string &mode);

template <class F>
const std::vector<uint8_t> to_bytes(const F &input) {
    std::vector<uint8_t> bytes(sizeof(input));
    std::memcpy(bytes.data(), &input, sizeof(input));

    return bytes;
} 

template <class F>
const F from_bytes(const std::vector<uint8_t> &bytes) {
    F val;
    std::memcpy(&val, bytes.data(), sizeof(F));
    return val;
}

template <FstQueryType Q, MapType M, class T> 
const std::vector<Q> tokens_from_map(const std::map<M, T> &tokens_map) {
    std::vector<Q> tokens;

    // Handle strings differently to all other M types
    if constexpr (std::is_same_v<M, std::string>) {
        for (auto &tok : tokens_map) {
            tokens.push_back(tok.first);
        }
    } else {
        for (auto &tok : tokens_map) {
            tokens.push_back(to_bytes(tok.first));
        }
    }

    return tokens;
}

#endif