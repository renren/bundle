#include <gtest/gtest.h>
#include <iostream>
#include "base3/basictypes.h"
#include "base3/cache.h"

using namespace base;

template<size_t size = 4>
struct Data {
  int val;
};

typedef Data<4> foo;

struct creator2 {
  foo* operator ()(int key) {
   
    foo*p = new foo;
    p->val = key;
    return p;
  }
};

foo* creator3(int) {
  return new foo;
}

template<class F> 
struct template_foo {
  template_foo(const F& f) : f_(f) {}
  F f_;
};

typedef foo* (*Creator)(int);

template<Creator c = 0>
struct bar {
  void call() {
    c(0);
  }
};

template<typename T, T* (*c)() = 0>
struct zoo {
  void call() {
    c();
  }
};

template<class F> 
template_foo<F> make_foo(const F& f) {
  return template_foo<F>(f);
}

struct monk {
  static foo* create(int) {
    return 0;
  }
};

TEST(CacheTest, None) {
#if 0
  HashCache<int, foo, creator2> c1(5);
  HashCache<int, foo, creator3> c2(5);
  foo s1 = c1.find(1);

  int arr[] = {1, 2, 3, 4, 5, 6, 1, 3};
  for (int i=0; i<sizeof(arr)/sizeof(arr[0]); ++i) {
    c1.find(arr[i]);

    c1.dump(std::cout);
    std::cout << "\n";
  }

  c1.dump(std::cout);
#endif

  make_foo(&creator3);

  Creator c = &creator3;

  bar<&creator3> bb;
  bb.call();

  HashCache<int, foo, creator3> hc1(1000);
  HashCache<int, foo, monk::create> hc2(1000);
}
