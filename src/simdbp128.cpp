#include "codecfactory.h"
#include "helper.h"
#include <vector>

using namespace SIMDCompressionLib;

// Library has own fuction to do this, but requires passing a string which is used as a hash key
// This is slightly faster, especially since it will be called potentially millions of times during encoding
std::shared_ptr<IntegerCODEC> get_codec(const uint8_t encoding) {
    switch (encoding) {
        case EncType::BPD1:
            return std::shared_ptr<IntegerCODEC>(new CompositeCodec<SIMDBinaryPacking<SIMDIntegratedBlockPacker<RegularDeltaSIMD, true>>, leftovercodec>());
        case EncType::BPD2:
            return std::shared_ptr<IntegerCODEC>(new CompositeCodec<SIMDBinaryPacking<SIMDIntegratedBlockPacker<CoarseDelta2SIMD, true>>, leftovercodec>());
        case EncType::BPD4:
            return std::shared_ptr<IntegerCODEC>(new CompositeCodec<SIMDBinaryPacking<SIMDIntegratedBlockPacker<CoarseDelta4SIMD, true>>, leftovercodec>());
        case EncType::BPDM:
            return std::shared_ptr<IntegerCODEC>(new CompositeCodec<SIMDBinaryPacking<SIMDIntegratedBlockPacker<Max4DeltaSIMD, true>>, leftovercodec>());
        default:
            throw std::invalid_argument("Encoding type must be a SIMDBP128 variant");

    }
}

std::vector<uint32_t> encode_simdbp128(std::vector<uint32_t> &input, const uint8_t encoding) {
    std::shared_ptr<IntegerCODEC> codec = get_codec(encoding);

    vector<uint32_t> encoded(input.size() + 1024);
    size_t encoded_size = encoded.size();
    codec->encodeArray(input.data(), input.size(), encoded.data(), encoded_size);

    encoded.resize(encoded_size);
    encoded.shrink_to_fit();

    return encoded;
}

std::vector<uint32_t> decode_simdbp128(std::vector<uint32_t> &input, const uint8_t encoding, size_t decoded_size) {
    std::shared_ptr<IntegerCODEC> codec = get_codec(encoding);

    vector<uint32_t> decoded(decoded_size);
    codec->decodeArray(input.data(), input.size(), decoded.data(), decoded_size);
    
    decoded.resize(decoded_size);
    decoded.shrink_to_fit();

    return decoded;
}