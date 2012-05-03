#ifndef BASE_BASE_STRING_ALGO_H__
#define BASE_BASE_STRING_ALGO_H__

#include <string>

// TODO:
// sha1, crc

namespace base {

std::string MD5(const std::string& data);

// sure: sizeof(digest) = 16
// void MD5(unsigned char* digest, const char* data, size_t length);
}
#endif // BASE_BASE_STRING_ALGO_H__
