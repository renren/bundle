#ifndef BUNDLE_SIXTY_H__
#define BUNDLE_SIXTY_H__

#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <stdint.h>

#include "base3/basictypes.h"

namespace bundle {

inline std::string ToSixty(int64_t number) {
	// we do not deal negative number, use -1 as magic code
	if (number < 0)
		return std::string();

  std::string result_str;
  // performance improve by reserve
  // ToSixy 100k clock: 36.545 ms
  // ToSixy 100k clock: 17.664 ms
  result_str.reserve(10);
  const int base = 60;

  const char SixtyChar[] = "ABCDEFGHJKLMNOPQRSTUVWXYZabcdefghjklmnopqrstuvwxyz0123456789";

  do {
    int m = number % base;
    result_str.push_back(SixtyChar[m]);
    number /= base;
  } while (number);

  std::reverse(result_str.begin(), result_str.end());
  return result_str;
}

// support 64bit sign integer
// max(int64_t) = 9223372036854775807
// in 60-format: QQOkhg5VQfH
// if error, return -1
inline int64_t FromSixty(const std::string& num_sixty) {
  // TODO: how to build this table?
  static int CharTable[] = {
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2
    , 3, 4, 5, 6, 7, -1, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21
    , 22, 23, 24, -1, -1, -1, -1, -1, -1, 25, 26, 27, 28, 29, 30, 31, 32, -1
    , 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49
  };
  static int64_t PowTable[] = {
    1, 60, 3600, 216000, 12960000,
    777600000, 46656000000LL, 2799360000000LL,
    167961600000000LL, 10077696000000000LL, 604661760000000000LL
  };

  int size = num_sixty.size();
  // lne(max 60 format) == 11
  if (size > 11 || size == 0)
    return -1;

  int64_t ten = 0;

  for (int j=0; j<size; j++) {
    char c = num_sixty[j];
    // alphabet check
    if (c < '0' || c > 'z')
      return -1;

    // ten += index * pow(60, position)
    //     A => 0
    //     B => 1
    //     a => 26
    //     b => 27

    int index = CharTable[c - '0'];
    ten += index * PowTable[size - j - 1];
    if (ten < 0)
      return -1;
  }

  return ten;
}

}
#endif // BUNDLE_SIXTY_H__
