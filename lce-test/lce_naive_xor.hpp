/*******************************************************************************
 * lce-text/lce_naive_xor.hpp
 *
 * Copyright (C) 2022 Patrick Dinklage <patrick.dinklage@tu-dortmund.de>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#pragma once

#include <bit>
#include <cstdint>
#include <vector>

#include <tlx/define/likely.hpp>

#include "util/lce_interface.hpp"

/* This class stores a text as an array of characters and 
 * answers LCE-queries with the naive method enhanced by replacing the
 * final scan of the mismatching block with an XOR / tzcnt. */

class LceNaiveXor : public LceDataStructure {
public:
  __extension__ typedef unsigned __int128 uint128_t;

  LceNaiveXor(std::vector<uint8_t> const& text)
    : text_(text), text_length_in_bytes_(text.size()) { }

  /* Naive LCE-query */
  uint64_t lce(const uint64_t i, const uint64_t j) {

    if (TLX_UNLIKELY(i == j)) {
      return text_length_in_bytes_ - i;
    }

    const uint64_t max_length = text_length_in_bytes_ - ((i < j) ? j : i);

    // First we compare the first few characters. We do this, because in the
    // usual case the lce is low.
    if(TLX_UNLIKELY(max_length <= 8)) {
      // do an "ultranaive" scan, there are at most 16 characters anyway
      uint64_t lce = 0;
      for(; lce < 8; ++lce) {
        if(TLX_UNLIKELY(lce >= max_length)) {
          return max_length;
        }
        if(text_[i + lce] != text_[j + lce]) {
          break;
        }
      }
      return lce;
    } else {
      // XOR the two 8-byte blocks
      uint64_t const x = *reinterpret_cast<uint64_t const*>(text_.data() + i) ^ *reinterpret_cast<uint64_t const*>(text_.data() + j);
      // if there is a mismatch, count the number of trailing zeroes to determine its location
      // we use the builtin rather than C++20's <bit> tools, because
      // those do a zero check we need not do
      if(x) return __builtin_ctzll(x) / 8;
    }

    // Accelerate search by comparing 16-byte blocks
    // we already know the first blocks are equal
    uint64_t lce = 0;
    uint128_t const* const text_blocks_i =
      reinterpret_cast<uint128_t const*>(text_.data() + i);
    uint128_t const * const text_blocks_j =
      reinterpret_cast<uint128_t const *>(text_.data() + j);
    for(; lce < max_length/16; ++lce) {
      if(text_blocks_i[lce] != text_blocks_j[lce]) {
        break;
      }
    }
    uint64_t const final_block = lce;
    lce *= 16;
    
    // The last block did not match.
    if(TLX_UNLIKELY(max_length < lce + 16)) {
      // it's also the last block in the text, do "ultranaive" scan
      for (; lce < max_length; ++lce) {
        if(text_[i + lce] != text_[j + lce]) {
          break;
        }
      }
      return lce;
    } else {
      uint128_t const x = text_blocks_i[final_block] ^ text_blocks_j[final_block];
      
      // we need not test x against zero, because we alread know there
      // is a mismatch. we just need to find where
      // assert(x != 0);
      if(static_cast<uint64_t>(x)) {
        // the mismatch is in the low bits
        // we use the builtin rather than C++20 <bit>, because
        // the latter tests the subject against zero first, which
        // is not needed here
        return lce + __builtin_ctzll(static_cast<uint64_t>(x)) / 8;
      } else {
        // the mismatch must be in the high bits
        // again, we use the builtin to avoid a branch
        return lce + 8 + __builtin_ctzll(*(reinterpret_cast<uint64_t const*>(&x)+1)) / 8;
      }
    }

  }

  inline char operator[](const uint64_t i) {
    return text_[i];
  }

  int isSmallerSuffix(const uint64_t i, const uint64_t j) {
    uint64_t lce_s = lce(i, j);
    if(TLX_UNLIKELY((i + lce_s + 1 == text_length_in_bytes_) ||
                (j + lce_s + 1 == text_length_in_bytes_))) {
      return true;
    }
    return (text_[i + lce_s] < text_[j + lce_s]);
  }

  uint64_t getSizeInBytes() {
    return text_length_in_bytes_;
  }

private: 
  std::vector<uint8_t> const& text_;
  const uint64_t text_length_in_bytes_;
};

/******************************************************************************/
