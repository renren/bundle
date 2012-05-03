// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base3/string_util.h"

#include "base3/build_config.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include <algorithm>
#include <vector>

#include "base3/basictypes.h"
#include "base3/logging.h"
// #include "base3/singleton.h"
// #include "base3/third_party/dmg_fp/dmg_fp.h"
// #include "base3/utf_string_conversion_utils.h"
// #include "base3/utf_string_conversions.h"
// #include "base3/third_party/icu/icu_utf.h"

namespace {

// Force the singleton used by Empty[W]String[16] to be a unique type. This
// prevents other code that might accidentally use Singleton<string> from
// getting our internal one.
struct EmptyStrings {
  EmptyStrings() {}
  const std::string s;
  const std::wstring ws;
  // const string16 s16;
};

// Used by ReplaceStringPlaceholders to track the position in the string of
// replaced parameters.
struct ReplacementOffset {
  ReplacementOffset(uintptr_t parameter, size_t offset)
      : parameter(parameter),
        offset(offset) {}

  // Index of the parameter.
  uintptr_t parameter;

  // Starting position in the string.
  size_t offset;
};

static bool CompareParameter(const ReplacementOffset& elem1,
                             const ReplacementOffset& elem2) {
  return elem1.parameter < elem2.parameter;
}

}  // namespace

namespace base {

bool IsWprintfFormatPortable(const wchar_t* format) {
  for (const wchar_t* position = format; *position != '\0'; ++position) {
    if (*position == '%') {
      bool in_specification = true;
      bool modifier_l = false;
      while (in_specification) {
        // Eat up characters until reaching a known specifier.
        if (*++position == '\0') {
          // The format string ended in the middle of a specification.  Call
          // it portable because no unportable specifications were found.  The
          // string is equally broken on all platforms.
          return true;
        }

        if (*position == 'l') {
          // 'l' is the only thing that can save the 's' and 'c' specifiers.
          modifier_l = true;
        } else if (((*position == 's' || *position == 'c') && !modifier_l) ||
                   *position == 'S' || *position == 'C' || *position == 'F' ||
                   *position == 'D' || *position == 'O' || *position == 'U') {
          // Not portable.
          return false;
        }

        if (wcschr(L"diouxXeEfgGaAcspn%", *position)) {
          // Portable, keep scanning the rest of the format string.
          in_specification = false;
        }
      }
    }
  }

  return true;
}

}  // namespace base

const EmptyStrings& SingletonEmptyStrings() {
  static EmptyStrings s_;
  return s_;
}

const std::string& EmptyString() {
  return SingletonEmptyStrings().s;
}


const std::wstring& EmptyWString() {
  return SingletonEmptyStrings().ws;
}

// const string16& EmptyString16() {
//   return Singleton<EmptyStrings>::get()->s16;
// }

#define WHITESPACE_UNICODE \
  0x0009, /* <control-0009> to <control-000D> */ \
  0x000A,                                        \
  0x000B,                                        \
  0x000C,                                        \
  0x000D,                                        \
  0x0020, /* Space */                            \
  0x0085, /* <control-0085> */                   \
  0x00A0, /* No-Break Space */                   \
  0x1680, /* Ogham Space Mark */                 \
  0x180E, /* Mongolian Vowel Separator */        \
  0x2000, /* En Quad to Hair Space */            \
  0x2001,                                        \
  0x2002,                                        \
  0x2003,                                        \
  0x2004,                                        \
  0x2005,                                        \
  0x2006,                                        \
  0x2007,                                        \
  0x2008,                                        \
  0x2009,                                        \
  0x200A,                                        \
  0x200C, /* Zero Width Non-Joiner */            \
  0x2028, /* Line Separator */                   \
  0x2029, /* Paragraph Separator */              \
  0x202F, /* Narrow No-Break Space */            \
  0x205F, /* Medium Mathematical Space */        \
  0x3000, /* Ideographic Space */                \
  0

const wchar_t kWhitespaceWide[] = {
  WHITESPACE_UNICODE
};
// const char16 kWhitespaceUTF16[] = {
//   WHITESPACE_UNICODE
// };


const char kWhitespaceASCII[] = {
  0x09,    // <control-0009> to <control-000D>
  0x0A,
  0x0B,
  0x0C,
  0x0D,
  0x20,    // Space
  0
};

const char kUtf8ByteOrderMark[] = "\xEF\xBB\xBF";

template<typename STR>
bool RemoveCharsT(const STR& input,
                  const typename STR::value_type remove_chars[],
                  STR* output) {
  bool removed = false;
  size_t found;

  *output = input;

  found = output->find_first_of(remove_chars);
  while (found != STR::npos) {
    removed = true;
    output->replace(found, 1, STR());
    found = output->find_first_of(remove_chars, found);
  }

  return removed;
}

bool RemoveChars(const std::wstring& input,
                 const wchar_t remove_chars[],
                 std::wstring* output) {
  return RemoveCharsT(input, remove_chars, output);
}

// #if !defined(WCHAR_T_IS_UTF16)
// bool RemoveChars(const string16& input,
//                  const char16 remove_chars[],
//                  string16* output) {
//   return RemoveCharsT(input, remove_chars, output);
// }
// #endif

bool RemoveChars(const std::string& input,
                 const char remove_chars[],
                 std::string* output) {
  return RemoveCharsT(input, remove_chars, output);
}

template<typename STR>
TrimPositions TrimStringT(const STR& input,
                          const typename STR::value_type trim_chars[],
                          TrimPositions positions,
                          STR* output) {
  // Find the edges of leading/trailing whitespace as desired.
  const typename STR::size_type last_char = input.length() - 1;
  const typename STR::size_type first_good_char = (positions & TRIM_LEADING) ?
      input.find_first_not_of(trim_chars) : 0;
  const typename STR::size_type last_good_char = (positions & TRIM_TRAILING) ?
      input.find_last_not_of(trim_chars) : last_char;

  // When the string was all whitespace, report that we stripped off whitespace
  // from whichever position the caller was interested in.  For empty input, we
  // stripped no whitespace, but we still need to clear |output|.
  if (input.empty() ||
      (first_good_char == STR::npos) || (last_good_char == STR::npos)) {
    bool input_was_empty = input.empty();  // in case output == &input
    output->clear();
    return input_was_empty ? TRIM_NONE : positions;
  }

  // Trim the whitespace.
  *output =
      input.substr(first_good_char, last_good_char - first_good_char + 1);

  // Return where we trimmed from.
  return static_cast<TrimPositions>(
      ((first_good_char == 0) ? TRIM_NONE : TRIM_LEADING) |
      ((last_good_char == last_char) ? TRIM_NONE : TRIM_TRAILING));
}

bool TrimString(const std::wstring& input,
                const wchar_t trim_chars[],
                std::wstring* output) {
  return TrimStringT(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}

bool TrimString(const std::string& input,
                const char trim_chars[],
                std::string* output) {
  return TrimStringT(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}


TrimPositions TrimWhitespace(const std::wstring& input,
                             TrimPositions positions,
                             std::wstring* output) {
  return TrimStringT(input, kWhitespaceWide, positions, output);
}

// #if !defined(WCHAR_T_IS_UTF16)
// TrimPositions TrimWhitespace(const string16& input,
//                              TrimPositions positions,
//                              string16* output) {
//   return TrimStringT(input, kWhitespaceUTF16, positions, output);
// }
// #endif

TrimPositions TrimWhitespaceASCII(const std::string& input,
                                  TrimPositions positions,
                                  std::string* output) {
  return TrimStringT(input, kWhitespaceASCII, positions, output);
}

// This function is only for backward-compatibility.
// To be removed when all callers are updated.
TrimPositions TrimWhitespace(const std::string& input,
                             TrimPositions positions,
                             std::string* output) {
  return TrimWhitespaceASCII(input, positions, output);
}



template<typename Iter>
static inline bool DoLowerCaseEqualsASCII(Iter a_begin,
                                          Iter a_end,
                                          const char* b) {
  for (Iter it = a_begin; it != a_end; ++it, ++b) {
    if (!*b || base::ToLowerASCII(*it) != *b)
      return false;
  }
  return *b == 0;
}

// Front-ends for LowerCaseEqualsASCII.
bool LowerCaseEqualsASCII(const std::string& a, const char* b) {
  return DoLowerCaseEqualsASCII(a.begin(), a.end(), b);
}

bool LowerCaseEqualsASCII(const std::wstring& a, const char* b) {
  return DoLowerCaseEqualsASCII(a.begin(), a.end(), b);
}

// #if !defined(WCHAR_T_IS_UTF16)
// bool LowerCaseEqualsASCII(const string16& a, const char* b) {
//   return DoLowerCaseEqualsASCII(a.begin(), a.end(), b);
// }
// #endif

bool LowerCaseEqualsASCII(std::string::const_iterator a_begin,
                          std::string::const_iterator a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool LowerCaseEqualsASCII(std::wstring::const_iterator a_begin,
                          std::wstring::const_iterator a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

// #if !defined(WCHAR_T_IS_UTF16)
// bool LowerCaseEqualsASCII(string16::const_iterator a_begin,
//                           string16::const_iterator a_end,
//                           const char* b) {
//   return DoLowerCaseEqualsASCII(a_begin, a_end, b);
// }
// #endif

bool LowerCaseEqualsASCII(const char* a_begin,
                          const char* a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool LowerCaseEqualsASCII(const wchar_t* a_begin,
                          const wchar_t* a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

// #if !defined(WCHAR_T_IS_UTF16)
// bool LowerCaseEqualsASCII(const char16* a_begin,
//                           const char16* a_end,
//                           const char* b) {
//   return DoLowerCaseEqualsASCII(a_begin, a_end, b);
// }
// #endif

// bool EqualsASCII(const string16& a, const base::StringPiece& b) {
//   if (a.length() != b.length())
//     return false;
//   return std::equal(b.begin(), b.end(), a.begin());
// }

bool StartsWithASCII(const std::string& str,
                     const std::string& search,
                     bool case_sensitive) {
  if (case_sensitive)
    return str.compare(0, search.length(), search) == 0;
  else
    return base::strncasecmp(str.c_str(), search.c_str(), search.length()) == 0;
}

template <typename STR>
bool StartsWithT(const STR& str, const STR& search, bool case_sensitive) {
  if (case_sensitive) {
    return str.compare(0, search.length(), search) == 0;
  } else {
    if (search.size() > str.size())
      return false;
    return std::equal(search.begin(), search.end(), str.begin(),
                      base::CaseInsensitiveCompare<typename STR::value_type>());
  }
}

bool StartsWith(const std::wstring& str, const std::wstring& search,
                bool case_sensitive) {
  return StartsWithT(str, search, case_sensitive);
}

// #if !defined(WCHAR_T_IS_UTF16)
// bool StartsWith(const string16& str, const string16& search,
//                 bool case_sensitive) {
//   return StartsWithT(str, search, case_sensitive);
// }
// #endif

template <typename STR>
bool EndsWithT(const STR& str, const STR& search, bool case_sensitive) {
  typename STR::size_type str_length = str.length();
  typename STR::size_type search_length = search.length();
  if (search_length > str_length)
    return false;
  if (case_sensitive) {
    return str.compare(str_length - search_length, search_length, search) == 0;
  } else {
    return std::equal(search.begin(), search.end(),
                      str.begin() + (str_length - search_length),
                      base::CaseInsensitiveCompare<typename STR::value_type>());
  }
}

bool EndsWith(const std::string& str, const std::string& search,
              bool case_sensitive) {
  return EndsWithT(str, search, case_sensitive);
}

bool EndsWith(const std::wstring& str, const std::wstring& search,
              bool case_sensitive) {
  return EndsWithT(str, search, case_sensitive);
}

// #if !defined(WCHAR_T_IS_UTF16)
// bool EndsWith(const string16& str, const string16& search,
//               bool case_sensitive) {
//   return EndsWithT(str, search, case_sensitive);
// }
// #endif


// The following code is compatible with the OpenBSD lcpy interface.  See:
//   http://www.gratisoft.us/todd/papers/strlcpy.html
//   ftp://ftp.openbsd.org/pub/OpenBSD/src/lib/libc/string/{wcs,str}lcpy.c

namespace {

template <typename CHAR>
size_t lcpyT(CHAR* dst, const CHAR* src, size_t dst_size) {
  for (size_t i = 0; i < dst_size; ++i) {
    if ((dst[i] = src[i]) == 0)  // We hit and copied the terminating NULL.
      return i;
  }

  // We were left off at dst_size.  We over copied 1 byte.  Null terminate.
  if (dst_size != 0)
    dst[dst_size - 1] = 0;

  // Count the rest of the |src|, and return it's length in characters.
  while (src[dst_size]) ++dst_size;
  return dst_size;
}

}  // namespace

size_t base::strlcpy(char* dst, const char* src, size_t dst_size) {
  return lcpyT<char>(dst, src, dst_size);
}
size_t base::wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size) {
  return lcpyT<wchar_t>(dst, src, dst_size);
}
