#include <gtest/gtest.h>
#include "base3/stringencode.h"
#include "base3/common.h"

using namespace base;

TEST(StringEncodeTest, Utf8Decode) {
  const struct Utf8Test {
    const char* encoded;
    size_t encsize, enclen;
    unsigned long decoded;
  } kTests[] = {
    { "a    ",             5, 1, 'a' },
    { "\x7F    ",          5, 1, 0x7F },
    { "\xC2\x80   ",       5, 2, 0x80 },
    { "\xDF\xBF   ",       5, 2, 0x7FF },
    { "\xE0\xA0\x80  ",    5, 3, 0x800 },
    { "\xEF\xBF\xBF  ",    5, 3, 0xFFFF },
    { "\xF0\x90\x80\x80 ", 5, 4, 0x10000 },
    { "\xF0\x90\x80\x80 ", 3, 0, 0x10000 },
    { "\xF0\xF0\x80\x80 ", 5, 0, 0 },
    { "\xF0\x90\x80  ",    5, 0, 0 },
    { "\x90\x80\x80  ",    5, 0, 0 },
    { NULL, 0, 0 },
  };

  for (size_t i=0; kTests[i].encoded; ++i) {
    unsigned long val = 0;
    EXPECT_TRUE(kTests[i].enclen == utf8_decode(kTests[i].encoded,
                                           kTests[i].encsize,
                                           &val));
    unsigned long result = (kTests[i].enclen == 0) ? 0 : kTests[i].decoded;
    EXPECT_TRUE(val == result);

    if (kTests[i].decoded == 0) {
      // Not an interesting encoding test case
      continue;
    }

    char buffer[5];
    memset(buffer, 0x01, ARRAY_SIZE(buffer));
    EXPECT_TRUE(kTests[i].enclen == utf8_encode(buffer,
                                           kTests[i].encsize,
                                           kTests[i].decoded));
    EXPECT_TRUE(memcmp(buffer, kTests[i].encoded, kTests[i].enclen) == 0);
#if 0
    // TODO: use EXPECT_TRUE
    // Make sure remainder of buffer is unchanged
    EXPECT_TRUE(memory_check(buffer + kTests[i].enclen,
                        0x1,
                        ARRAY_SIZE(buffer) - kTests[i].enclen));
#endif
  }
}
