#ifndef AVX2_DELTA_H
#define AVX2_DELTA_H

#if defined(SIMD_TARGET_AVX2)

#include <immintrin.h>
#include <stdint.h>
#include <vector>

#include "codecs.h"
#include "util.h"

using namespace SIMDCompressionLib;

// Modification of CourseDelta4 to run on 256-bit simd
// Compares deltas with a gap of 8
// https://github.com/fast-pack/SIMDCompressionAndIntersection/blob/master/include/deltatemplates.h
struct CoarseDelta4AVX2 {
  static __m256i PrefixSum(__m256i curr, __m256i prev) {
    return _mm256_add_epi32(curr, prev);
  }

  static __m256i Delta(__m256i curr, __m256i prev) {
    return _mm256_sub_epi32(curr, prev);
  }
};

// Modification of Max4Delta to run on 256-bit simd
// Compares deltas with a maximum gap of 8
// https://github.com/fast-pack/SIMDCompressionAndIntersection/blob/master/include/deltatemplates.h
struct Max4DeltaAVX2 {
  static __m256i PrefixSum(__m256i curr, __m256i prev) {
    return _mm256_add_epi32(curr, _mm256_permute4x64_epi64(prev, 0xff));
  }

  static __m256i Delta(__m256i curr, __m256i prev) {
    return _mm256_sub_epi32(curr, _mm256_permute4x64_epi64(prev, 0xff));
  }
};

// Loses data if input size is not a multiple of 8
// Function only used in benchmarking so the lack of edge case handling
// is not important here
template <class DeltaHelper> void s8_delta_encode(const std::vector<uint32_t> &in, std::vector<uint32_t> &out) {
  const size_t in_size = in.size();
  __m256i prev = _mm256_setzero_si256();

  size_t i = 0;
  for (; i + 8 <= in_size; i += 8) {
    const __m256i curr = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&in[i]));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(&out[i]), DeltaHelper::Delta(curr, prev));
    prev = curr;
  }
}

// Loses data if input size is not a multiple of 8
// Function only used in benchmarking so the lack of edge case handling
// is not important here
template <class DeltaHelper> void s8_delta_decode(const std::vector<uint32_t> &in, std::vector<uint32_t> &out) {
  const size_t in_size = in.size();
  __m256i prev = _mm256_setzero_si256();
  
  size_t i = 0;
  for (; i + 8 <= in_size; i += 8) {
    prev = DeltaHelper::PrefixSum(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(&in[i])), prev);
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(&out[i]), prev);
  }
}

#endif
#endif