// Copyright 2009 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// This class defines a fast, bit-vector mask for 8-bit unsigned characters.
// It internally stores 256 bits in a uint32 array of size 8, and can do quick
// bit-flicking to lookup needed characters.
//
// This class is thread-safe.

#ifndef COMMON_CHARMASK_H__
#define COMMON_CHARMASK_H__

#include "base3/basictypes.h"


class CharMask {
 public:
  // Initializes with given uint32 values.
  // For instance, the first variable contains bits for values
  // 0x0F (SI) down to 0x00 (NUL).
  CharMask(uint32 b0, uint32 b1, uint32 b2, uint32 b3,
           uint32 b4, uint32 b5, uint32 b6, uint32 b7) {
    mask_[0] = b0;
    mask_[1] = b1;
    mask_[2] = b2;
    mask_[3] = b3;
    mask_[4] = b4;
    mask_[5] = b5;
    mask_[6] = b6;
    mask_[7] = b7;
  }

  // Check the mask value for character "c".
  bool contains(unsigned char c) const {
    return (mask_[c >> 5] & (1 << (c & 31))) != 0;
  }

  // Set the bit mask.
  void set(unsigned char c, bool flag) {
    if (flag) {
      mask_[c >> 5] |= (1 << (c & 31));
    } else {
      mask_[c >> 5] &= ~(1 << (c & 31));
    }
  }

 private:
  uint32 mask_[8];
};

#endif  // COMMON_CHARMASK_H__
