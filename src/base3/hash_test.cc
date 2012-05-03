#include <gtest/gtest.h>
#include <fstream>
#include "base3/hash.h"

#ifdef OS_WIN
#define snprintf _snprintf
#endif

void CheckHash (const char* string, const char* correct_digest) {
  EXPECT_TRUE(correct_digest);
  EXPECT_TRUE(string);

  using namespace base;
  uint32 h = MurmurHash2(string, strlen(string), 0);
  
  const int digest_len = 32+1;
  char digest_string[digest_len] = {0};

  snprintf(digest_string, digest_len, "%x", h);

  std::cout << string << " " << std::string(digest_string) << " " << correct_digest << "\n";
  EXPECT_EQ(std::string(digest_string), correct_digest);
}

TEST(Hash, Murmur) {
  CheckHash ("", "0");
  CheckHash ("a", "92685f5e");
  CheckHash ("abc", "13577c9b");
  CheckHash ("message digest", "86f9b82");
  CheckHash ("abcdefghijklmnopqrstuvwxyz", "aa286635");
  CheckHash ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "e2edf478");
  CheckHash ("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "21dac0da");

  std::cout << std::hex << ('a' << 16) << "\n";
  std::cout << std::hex << (3^('a' << 16)) << "\n";
  std::cout << std::hex << ('a' << 16) * 0x5bd1e995 << "\n";
  std::cout << std::hex << 610000 * 1540483477 << "\n";
  // std::cout << std::hex << ((3 ^ ('a' << 16)) * 0x5bd1e995) << std::endl;
}


TEST(HashTest, Hash) {
#if 0
  if (argc == 1)
    std::cout << base::MurmurHash2(argv[1], strlen(argv[1]), 0) << std::endl;
  else {
    std::ifstream istem(argv[2], std::ios::binary);
    istem.unsetf(std::ios::skipws);

    std::string data;

    std::copy(
      std::istream_iterator<char>(istem), std::istream_iterator<char>()
      , std::back_inserter(data)
      );

    std::cout << base::MurmurHash2(data.data(), data.size(), 0) << std::endl;
  }
#endif
}