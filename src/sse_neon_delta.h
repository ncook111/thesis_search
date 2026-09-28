#ifndef SSE_NEON_DELTA_H
#define SSE_NEON_DELTA_H

#if defined(SIMD_TARGET_NEON) || defined(SIMD_TARGET_SSE)

template <class DeltaHelper> void s4_delta_encode(const uint32_t *in, uint32_t *out, size_t size) {
  __m128i prev = _mm_setzero_si128();
  for (size_t i = 0; i + 4 <= size; i += 4) {
    const __m128i curr = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&in[i]));
    _mm_storeu_si128(reinterpret_cast<__m128i *>(&out[i]), DeltaHelper::Delta(curr, prev));
    prev = curr;
  }
}

template <class DeltaHelper> void s4_delta_decode(const uint32_t *in, uint32_t *out, size_t size) {
  __m128i prev = _mm_setzero_si128();
  for (size_t i = 0; i + 4 <= size; i += 4) {
    prev = DeltaHelper::PrefixSum(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&in[i])), prev);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(&out[i]), prev);
  }
}

#endif
#endif