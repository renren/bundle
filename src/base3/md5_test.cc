// Copyright 2004-2009 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file ebasept in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ========================================================================
// MD5 unittest

#include <gtest/gtest.h>
#include "base3/md5.h"

#ifdef OS_WIN
#define snprintf _snprintf
#endif

void CheckMD5 (const char* string, const char* correct_digest) {
  EXPECT_TRUE(correct_digest);
  EXPECT_TRUE(string);

  MD5Context context;
  MD5Init (&context);
  MD5Update (&context, (unsigned char *)string, strlen (string));
  MD5Digest digest;
  MD5Final (&digest, &context);

  const int digest_len = 32+1;
  char digest_string[digest_len] = {0};
  digest_string[31] = '\0';
  digest_string[0] = '\0';

  for (int i = 0; i < 16; i++) {
    int n = strlen(digest_string);
    snprintf(digest_string + n, digest_len - n, "%02x", digest.a[i]);
  }

  std::cout << std::string(digest_string) << " " << correct_digest << "\n";
  EXPECT_TRUE(std::string(digest_string) == correct_digest);
}

TEST(Md5Test, ResultCheck) {
  CheckMD5 ("", "d41d8cd98f00b204e9800998ecf8427e");
  CheckMD5 ("a", "0cc175b9c0f1b6a831c399e269772661");
  CheckMD5 ("abc", "900150983cd24fb0d6963f7d28e17f72");
  CheckMD5 ("message digest", "f96b697d7cb7938d525a2f31aaf161d0");
  CheckMD5 ("abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b");
  CheckMD5 ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f");
  CheckMD5 ("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a");
}
