#include "codecfactory.h"
#include "helper.h"
#include "coder.h"
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

uint64_t write_bp128_to_file(std::vector<uint64_t> vec, std::ofstream &file, const uint8_t encoding) {
  size_t orig_size = vec.size();

  std::vector<uint32_t> vec32(vec.begin(), vec.end());

  for (const auto v : vec) {
    if (v > UINT32_MAX) {
      throw std::runtime_error("uint32_t conversion overflow");
    }
  }
  const std::vector<uint32_t> enc_data = encode_simdbp128(vec32, encoding);
  const uint64_t enc_data_size = enc_data.size() * sizeof(uint32_t);

  std::vector<uint8_t> enc_orig_size = vb_encode_num_uint64_t(orig_size);
  const uint64_t enc_orig_size_size = enc_orig_size.size();
  
  file.write(reinterpret_cast<const char *>(enc_orig_size.data()), enc_orig_size_size);
  file.write(reinterpret_cast<const char *>(enc_data.data()), enc_data_size);

  return enc_data_size + enc_orig_size_size;
}

std::vector<uint64_t> read_bp128_from_file(const uint8_t *file, const size_t size_to_read, const uint8_t encoding) {
  size_t eos_size_to_read = std::min({size_to_read, (size_t)10}); // orig_size can be up to 10 bytes, get lesser of size_to_read or 10
  std::vector<uint8_t> enc_orig_size(file, file + eos_size_to_read); 

  size_t num_bytes;
  size_t orig_size = vb_decode_num_uint64_t(enc_orig_size, num_bytes);

  std::vector<uint32_t> encoded((size_to_read - num_bytes) / sizeof(uint32_t));
  std::memcpy(reinterpret_cast<char *>(encoded.data()), file + num_bytes, size_to_read - num_bytes);  

  const std::vector<uint32_t> decoded = decode_simdbp128(encoded, encoding, orig_size);

  return std::vector<uint64_t>(decoded.begin(), decoded.end());
}