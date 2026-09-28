#ifndef AVX2_BINARY_PACKING_H
#define AVX2_BINARY_PACKING_H

#if defined(SIMD_TARGET_AVX2)

#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <string>

#include "codecs.h"
#include "util.h"
#include "avx2_delta.h"

// simdcomp functions - declared this way instead of including above as they
// contain function names that are present within SIMDCompressionAndIntersection
extern "C" {
  void avxpackwithoutmask(const uint32_t *in, __m256i *out, const uint32_t bit);
  void avxunpack(const __m256i *in, uint32_t *out, const uint32_t bit);
}

using namespace SIMDCompressionLib;

// Modification of code from SIMDCompressionAndIntersection within include/simdbinarypacking.h
// https://github.com/fast-pack/SIMDCompressionAndIntersection/blob/master/include/simdbinarypacking.h

const uint32_t AVX2BlockSize = 256;

template <class DeltaHelper> struct SIMDBlockPackerAVX2 {
  static void unpackblock(const uint32_t *in, uint32_t *out, const uint32_t bit, __m256i &initoffset) {
    if (bit == 0) // simdcomp's avxunpack doesn't clear all values
      memset(out, 0, AVX2BlockSize * sizeof(uint32_t));
    else
      avxunpack(reinterpret_cast<const __m256i *>(in), out, bit);

    if (bit < 32) {
      __m256i *pout = reinterpret_cast<__m256i *>(out);
      for (uint32_t k = 0; 8 * k < AVX2BlockSize; ++k) {
        initoffset = DeltaHelper::PrefixSum(_mm256_loadu_si256(pout + k), initoffset);
        _mm256_storeu_si256(pout + k, initoffset);
      }
    } else {
      initoffset = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(out + AVX2BlockSize - 8));
    }
  }

  static uint32_t maxbits(const uint32_t *in, __m256i &initoffset) {
    const __m256i *pin = reinterpret_cast<const __m256i *>(in);
    __m256i newvec = _mm256_loadu_si256(pin);
    __m256i accumulator = DeltaHelper::Delta(newvec, initoffset);
    __m256i oldvec = newvec;

    for (uint32_t k = 1; 8 * k < AVX2BlockSize; ++k) {
      newvec = _mm256_loadu_si256(pin + k);
      accumulator = _mm256_or_si256(accumulator, DeltaHelper::Delta(newvec, oldvec));
      oldvec = newvec;
    }

    initoffset = oldvec;
    return maxbitas32int(_mm_or_si128(_mm256_castsi256_si128(accumulator), _mm256_extracti128_si256(accumulator, 1)));
    }

  static void packblockwithoutmask(const uint32_t *in, uint32_t *out, const uint32_t bit, __m256i &initoffset) {
    const __m256i *pin = reinterpret_cast<const __m256i *>(in);
    const __m256i nextoffset = _mm256_loadu_si256(pin + AVX2BlockSize / 8 - 1);

    if (bit < 32) {
      SIMDCOMP_ALIGNED(32) uint32_t deltas[AVX2BlockSize];
      __m256i *pdeltas = reinterpret_cast<__m256i *>(deltas);
      __m256i prev = initoffset;

      for (uint32_t k = 0; 8 * k < AVX2BlockSize; ++k) {
        const __m256i curr = _mm256_loadu_si256(pin + k);
        _mm256_store_si256(pdeltas + k, DeltaHelper::Delta(curr, prev));
        prev = curr;
      }

      avxpackwithoutmask(deltas, reinterpret_cast<__m256i *>(out), bit);
    } else {
      avxpackwithoutmask(in, reinterpret_cast<__m256i *>(out), bit);
    }

    initoffset = nextoffset;
    }
};
    
template <class BlockPacker> class SIMDBinaryPackingAVX2 : public IntegerCODEC {
public:
  static const uint32_t MiniBlockSize = AVX2BlockSize;
  static const uint32_t HowManyMiniBlocks = 16;
  static const uint32_t BlockSize = MiniBlockSize; // HowManyMiniBlocks * MiniBlockSize;
    
  void encodeArray(uint32_t *in, const size_t length, uint32_t *out, size_t &nvalue) {
    checkifdivisibleby(length, BlockSize);
    const uint32_t *const initout(out);
    *out++ = static_cast<uint32_t>(length);
    uint32_t Bs[HowManyMiniBlocks];
    __m256i init = _mm256_setzero_si256();
    const uint32_t *const final = in + length;
        
    for (; in + HowManyMiniBlocks * MiniBlockSize <= final; in += HowManyMiniBlocks * MiniBlockSize) {
      __m256i tmpinit = init;
      for (uint32_t i = 0; i < HowManyMiniBlocks; ++i) {
          Bs[i] = BlockPacker::maxbits(in + i * MiniBlockSize, tmpinit);
      }

      *out++ = (Bs[0] << 24) | (Bs[1] << 16) | (Bs[2] << 8) | Bs[3];
      *out++ = (Bs[4] << 24) | (Bs[5] << 16) | (Bs[6] << 8) | Bs[7];
      *out++ = (Bs[8] << 24) | (Bs[9] << 16) | (Bs[10] << 8) | Bs[11];
      *out++ = (Bs[12] << 24) | (Bs[13] << 16) | (Bs[14] << 8) | Bs[15];
      for (uint32_t i = 0; i < HowManyMiniBlocks; ++i) {
        BlockPacker::packblockwithoutmask(in + i * MiniBlockSize, out, Bs[i], init);
        out += MiniBlockSize / 32 * Bs[i];
      }
    }

    if (in < final) {
      const size_t howmany = (final - in) / MiniBlockSize;
      __m256i tmpinit = init;
      memset(&Bs[0], 0, HowManyMiniBlocks * sizeof(uint32_t));

      for (uint32_t i = 0; i < howmany; ++i) {
          Bs[i] = BlockPacker::maxbits(in + i * MiniBlockSize, tmpinit);
      }

      *out++ = (Bs[0] << 24) | (Bs[1] << 16) | (Bs[2] << 8) | Bs[3];
      *out++ = (Bs[4] << 24) | (Bs[5] << 16) | (Bs[6] << 8) | Bs[7];
      *out++ = (Bs[8] << 24) | (Bs[9] << 16) | (Bs[10] << 8) | Bs[11];
      *out++ = (Bs[12] << 24) | (Bs[13] << 16) | (Bs[14] << 8) | Bs[15];

      for (uint32_t i = 0; i < howmany; ++i) {
          BlockPacker::packblockwithoutmask(in + i * MiniBlockSize, out, Bs[i], init);
          out += MiniBlockSize / 32 * Bs[i];
      }

      in += howmany * MiniBlockSize;
    }

    nvalue = out - initout;
    }
    
  const uint32_t *decodeArray(const uint32_t *in, const size_t /*length*/, uint32_t *out, size_t &nvalue) {
    const uint32_t actuallength = *in++;
    const uint32_t *const initout(out);
    uint32_t Bs[HowManyMiniBlocks];
    __m256i init = _mm256_setzero_si256();

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
    return "SIMDBinaryPackingAVX2"; 
  }
};

#endif // SIMD_TARGET_AVX2
#endif // AVX2_BINARY_PACKING_H