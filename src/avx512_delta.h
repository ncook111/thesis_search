#ifndef AVX512_DELTA_H
#define AVX512_DELTA_H

#if defined(SIMD_TARGET_AVX512BW)

#include <immintrin.h>
#include <stdint.h>
#include <vector>

#include "codecs.h"
#include "util.h"

using namespace SIMDCompressionLib;

// Modification of CourseDelta4 to run on 512-bit simd
// Compares deltas with a gap of 16
// https://github.com/fast-pack/SIMDCompressionAndIntersection/blob/master/include/deltatemplates.h
struct CoarseDelta4AVX512 {
  static __m512i PrefixSum(__m512i curr, __m512i prev) {
    return _mm512_add_epi32(curr, prev);
  }

  static __m512i Delta(__m512i curr, __m512i prev) {
    return _mm512_sub_epi32(curr, prev);
  }
};

// Modification of Max4Delta to run on 256-bit simd
// Compares deltas with a maximum gap of 16
// https://github.com/fast-pack/SIMDCompressionAndIntersection/blob/master/include/deltatemplates.h
struct Max4DeltaAVX512 {
  static __m512i PrefixSum(__m512i curr, __m512i prev) {
    return _mm512_add_epi32(curr, _mm512_shuffle_i32x4(prev, prev, 0xff));
  }

  static __m512i Delta(__m512i curr, __m512i prev) {
    return _mm512_sub_epi32(curr, _mm512_shuffle_i32x4(prev, prev, 0xff));
  }
};

// Loses data if input size is not a multiple of 16
// Function only used in benchmarking so the lack of edge case handling
// is not important here
template <class DeltaHelper, class Vec> void s16_delta_encode(const std::vector<uint32_t> &in, Vec &out) {
  const size_t in_size = in.size();
  __m512i prev = _mm512_setzero_si512();

  size_t i = 0;
  for (; i + 16 <= in_size; i += 16) {
    const __m512i curr = _mm512_loadu_si512(&in[i]);
    _mm512_storeu_si512(&out[i], DeltaHelper::Delta(curr, prev));
    prev = curr;
  }
}

// Loses data if input size is not a multiple of 16
// Function only used in benchmarking so the lack of edge case handling
// is not important here
template <class DeltaHelper, class Vec> void s16_delta_decode(const Vec &in, Vec &out) {
  const size_t in_size = in.size();
  __m512i prev = _mm512_setzero_si512();
  
  size_t i = 0;
  for (; i + 16 <= in_size; i += 16) {
    prev = DeltaHelper::PrefixSum(_mm512_loadu_si512(&in[i]), prev);
    _mm512_storeu_si512(&out[i], prev);
  }
}

#endif
#endif