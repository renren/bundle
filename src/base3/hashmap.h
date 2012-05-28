#ifndef BASE_BASE_HASHMAP_H__
#define BASE_BASE_HASHMAP_H__

#ifdef __GNUC__
#include <stdint.h>

#include <ext/hash_map>
#include <ext/hash_set>

namespace __gnu_cxx {
  template<> struct hash<std::string> {
    size_t operator()( const std::string& x ) const {
      return hash< const char* >()( x.c_str() );
    }
  };

#if !defined(__LP64__)
  template<> struct hash<uint64_t> {
    size_t operator()( const uint64_t x ) const {
      return x;
    }
  };
#endif
}

namespace std {
  using __gnu_cxx::hash_map;
  using __gnu_cxx::hash_set;

  using __gnu_cxx::hash;
}

#elif defined(_MSC_VER)

#include <hash_map>
#include <hash_set>

namespace std {
  using stdext::hash_map;
  using stdext::hash_set;

#if 0
  // < MSC_VER 2010
  template<typename T>
  struct hash {
    size_t operator()(const T& t) {
      return stdext::hash_value(t);
    }
  };
#endif
}

#endif

// using TR1's 
// #include <unordered_map>
// typedef std::unordered_map<std::string, int> stringmap;
// typedef std::unordered_map<int, int> intmap;

#endif // BASE_BASE_HASHMAP_H__

