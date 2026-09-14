#ifndef SIMDBP128_H
#define SIMDBP128_H

#include "codecfactory.h"
#include "helper.h"
#include <vector>

using namespace SIMDCompressionLib;

std::shared_ptr<IntegerCODEC> get_codec(const uint8_t encoding);
std::vector<uint32_t> encode_simdbp128(std::vector<uint32_t> &input, const uint8_t encoding);
std::vector<uint32_t> decode_simdbp128(std::vector<uint32_t> &input, const uint8_t encoding, size_t decoded_size);

#endif