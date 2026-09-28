#ifndef AVX512_BINARY_PACKING_H
#define AVX512_BINARY_PACKING_H

#if defined(SIMD_TARGET_AVX512BW)

#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <string>

#include "codecs.h"
#include "util.h"
#include "avx512_delta.h"

// simdcomp functions - declared this way instead of including above as they
// contain function names that are present within SIMDCompressionAndIntersection
extern "C" {
  void avx512packwithoutmask(const uint32_t *in, __m512i *out, const uint32_t bit);
  void avx512unpack(const __m512i *in, uint32_t *out, const uint32_t bit);
}

using namespace SIMDCompressionLib;

// Modification of code from SIMDCompressionAndIntersection within include/simdbinarypacking.h
// https://github.com/fast-pack/SIMDCompressionAndIntersection/blob/master/include/simdbinarypacking.h

const uint32_t AVX512BlockSize = 512; // simdcomp's AVX512BlockSize
 
template <class DeltaHelper> struct SIMDBlockPackerAVX512 {
  static void unpackblock(const uint32_t *in, uint32_t *out, const uint32_t bit, __m512i &initoffset) {
    if (bit == 0) // simdcomp's avx512unpack doesn't clear all values
      memset(out, 0, AVX512BlockSize * sizeof(uint32_t));
    else
      avx512unpack(reinterpret_cast<const __m512i *>(in), out, bit);

    if (bit < 32) {
      __m512i *pout = reinterpret_cast<__m512i *>(out);
      for (uint32_t k = 0; 16 * k < AVX512BlockSize; ++k) {
        initoffset = DeltaHelper::PrefixSum(_mm512_loadu_si512(pout + k), initoffset);
        _mm512_storeu_si512(pout + k, initoffset);
      }
    } else {
      initoffset = _mm512_loadu_si512(out + AVX512BlockSize - 16);
    }
  }
 
  static uint32_t maxbits(const uint32_t *in, __m512i &initoffset) {
    const __m512i *pin = reinterpret_cast<const __m512i *>(in);
    __m512i newvec = _mm512_loadu_si512(pin);
    __m512i accumulator = DeltaHelper::Delta(newvec, initoffset);
    __m512i oldvec = newvec;

    for (uint32_t k = 1; 16 * k < AVX512BlockSize; ++k) {
      newvec = _mm512_loadu_si512(pin + k);
      accumulator = _mm512_or_si512(accumulator, DeltaHelper::Delta(newvec, oldvec));
      oldvec = newvec;
    }

    initoffset = oldvec;
    const __m256i acc256 = _mm256_or_si256(_mm512_castsi512_si256(accumulator), _mm512_extracti64x4_epi64(accumulator, 1));
    return maxbitas32int(_mm_or_si128(_mm256_castsi256_si128(acc256), _mm256_extracti128_si256(acc256, 1)));
  }
 
  static void packblockwithoutmask(const uint32_t *in, uint32_t *out, const uint32_t bit, __m512i &initoffset) {
    const __m512i *pin = reinterpret_cast<const __m512i *>(in);
    const __m512i nextoffset = _mm512_loadu_si512(pin + AVX512BlockSize / 16 - 1);
    
    if (bit < 32) {
      SIMDCOMP_ALIGNED(64) uint32_t deltas[AVX512BlockSize];
      __m512i *pdeltas = reinterpret_cast<__m512i *>(deltas);
      __m512i prev = initoffset;
      
      for (uint32_t k = 0; 16 * k < AVX512BlockSize; ++k) {
        const __m512i curr = _mm512_loadu_si512(pin + k);
        _mm512_store_si512(pdeltas + k, DeltaHelper::Delta(curr, prev));
        prev = curr;
      }

      avx512packwithoutmask(deltas, reinterpret_cast<__m512i *>(out), bit);
    } else {
      avx512packwithoutmask(in, reinterpret_cast<__m512i *>(out), bit);
    }

    initoffset = nextoffset;
  }
};
 
template <class BlockPacker> class SIMDBinaryPackingAVX512 : public IntegerCODEC {
public:
  static const uint32_t MiniBlockSize = AVX512BlockSize;
  static const uint32_t HowManyMiniBlocks = 16;
  static const uint32_t BlockSize = MiniBlockSize; // HowManyMiniBlocks * MiniBlockSize;
 
  void encodeArray(uint32_t *in, const size_t length, uint32_t *out, size_t &nvalue) {
    checkifdivisibleby(length, BlockSize);
    const uint32_t *const initout(out);
    *out++ = static_cast<uint32_t>(length);
    uint32_t Bs[HowManyMiniBlocks];
    __m512i init = _mm512_setzero_si512();
    const uint32_t *const final = in + length;
    
    for (; in + HowManyMiniBlocks * MiniBlockSize <= final; in += HowManyMiniBlocks * MiniBlockSize) {
      __m512i tmpinit = init;
      
      for (uint32_t i = 0; i < HowManyMiniBlocks; ++i) {
        Bs[i] = BlockPacker::maxbits(in + i * MiniBlockSize, tmpinit);
      }

      *out++ = (Bs[0] << 24) | (Bs[1] << 16) | (Bs[2] << 8) | Bs[3];
      *out++ = (Bs[4] << 24) | (Bs[5] << 16) | (Bs[6] << 8) | Bs[7];
      *out++ = (Bs[8] << 24) | (Bs[9] << 16) | (Bs[10] << 8) | Bs[11];
      *out++ = (Bs[12] << 24) | (Bs[13] << 16) | (Bs[14] << 8) | Bs[15];
      
      for (uint32_t i = 0; i < HowManyMiniBlocks; ++i) {
        BlockPacker::packblockwithoutmask(in + i * MiniBlockSize, out, Bs[i],
                                          init);
        out += MiniBlockSize / 32 * Bs[i];
      }
    }
    if (in < final) {
      const size_t howmany = (final - in) / MiniBlockSize;
      __m512i tmpinit = init;
      memset(&Bs[0], 0, HowManyMiniBlocks * sizeof(uint32_t));
      
      for (uint32_t i = 0; i < howmany; ++i) {
        Bs[i] = BlockPacker::maxbits(in + i * MiniBlockSize, tmpinit);
      }
      
      *out++ = (Bs[0] << 24) | (Bs[1] << 16) | (Bs[2] << 8) | Bs[3];
      *out++ = (Bs[4] << 24) | (Bs[5] << 16) | (Bs[6] << 8) | Bs[7];
      *out++ = (Bs[8] << 24) | (Bs[9] << 16) | (Bs[10] << 8) | Bs[11];
      *out++ = (Bs[12] << 24) | (Bs[13] << 16) | (Bs[14] << 8) | Bs[15];
      
      for (uint32_t i = 0; i < howmany; ++i) {
        BlockPacker::packblockwithoutmask(in + i * MiniBlockSize, out, Bs[i],
                                          init);
        out += MiniBlockSize / 32 * Bs[i];
      }

      in += howmany * MiniBlockSize;
    }

    nvalue = out - initout;
  }
 
  const uint32_t *decodeArray(const uint32_t *in, const size_t /*length*/,
                              uint32_t *out, size_t &nvalue) {
    const uint32_t actuallength = *in++;
    const uint32_t *const initout(out);
    uint32_t Bs[HowManyMiniBlocks];
    __m512i init = _mm512_setzero_si512();
    
    for (; out < initout + actuallength / (HowManyMiniBlocks * MiniBlockSize) * HowManyMiniBlocks * MiniBlockSize; out += HowManyMiniBlocks * MiniBlockSize) {
      for (uint32_t i = 0; i < 4; ++i, ++in) {
        Bs[0 + 4 * i] = static_cast<uint8_t>(in[0] >> 24);
        Bs[1 + 4 * i] = static_cast<uint8_t>(in[0] >> 16);
        Bs[2 + 4 * i] = static_cast<uint8_t>(in[0] >> 8);
        Bs[3 + 4 * i] = static_cast<uint8_t>(in[0]);
      }

      for (uint32_t i = 0; i < HowManyMiniBlocks; ++i) {
        BlockPacker::unpackblock(in, out + i * MiniBlockSize, Bs[i], init);
        in += MiniBlockSize / 32 * Bs[i];
      }
    }
 
    if (out < initout + actuallength) {
      const size_t howmany = (initout + actuallength - out) / MiniBlockSize;
      
      for (uint32_t i = 0; i < 4; ++i, ++in) {
        Bs[0 + 4 * i] = static_cast<uint8_t>(in[0] >> 24);
        Bs[1 + 4 * i] = static_cast<uint8_t>(in[0] >> 16);
        Bs[2 + 4 * i] = static_cast<uint8_t>(in[0] >> 8);
        Bs[3 + 4 * i] = static_cast<uint8_t>(in[0]);
      }

      for (uint32_t i = 0; i < howmany; ++i) {
        BlockPacker::unpackblock(in, out + i * MiniBlockSize, Bs[i], init);
        in += MiniBlockSize / 32 * Bs[i];
      }

      out += howmany * MiniBlockSize;
    }
    nvalue = out - initout;
    return in;
  }

  std::string name() const { 
    return "SIMDBinaryPackingAVX512"; 
  }
};

#endif // SIMD_TARGET_AVX512BW
#endif // AVX512_BINARY_PACKING_H