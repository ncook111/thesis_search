#ifndef SIMDBP128_H
#define SIMDBP128_H

#include "helper.h"
#include "codecfactory.h"
#include <vector>

using namespace SIMDCompressionLib;

std::shared_ptr<IntegerCODEC> get_codec(const uint8_t encoding);
std::vector<uint32_t> encode_simdbp128(const std::vector<uint32_t> &input, const uint8_t encoding);
std::vector<uint32_t> decode_simdbp128(const std::vector<uint32_t> &input, const uint8_t encoding, size_t decoded_size);
void encode_simdbp128(const std::vector<uint32_t> &input, std::vector<uint32_t> &output, const uint8_t encoding);
void decode_simdbp128(const std::vector<uint32_t> &input, std::vector<uint32_t> &output, const uint8_t encoding, size_t decoded_size);
uint64_t write_bp128_to_file(const std::vector<uint32_t> &vec, std::ostream &file, const uint8_t encoding);
std::vector<uint32_t> read_bp128_from_file(const uint8_t *file, const size_t size_to_read, const uint8_t encoding);

#endif