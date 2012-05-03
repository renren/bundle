#ifndef BASE_CACHE_CLIENT_CONSISTENT_H__
#define BASE_CACHE_CLIENT_CONSISTENT_H__

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <limits>
#include <boost/foreach.hpp>
#include "base3/basictypes.h"
#include "base3/stringdigest.h"
#include "base3/hash.h"
#include "base3/rwlock.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

namespace base {

class Continuum {
public:
  std::string Locate(uint32 hash) const {
    // TODO: lock
    std::string desc;
    lock_.ReadLock();
    PointList::const_iterator i = std::lower_bound(points_.begin(), points_.end()
      , Point(hash, ""), CompareByPoint());
    if (i != points_.end())
      desc = i->desc_;
    else {
      if (!points_.empty())
        desc = points_[0].desc_;
    }
    lock_.ReadUnlock();

    return desc;
  }

  static uint32 Hash(const char* text, size_t length) {
    return MurmurHash2(text, length, 0);
  }
  static uint32 Hash(const std::string& text) {
    return MurmurHash2(text.c_str(), text.size(), 0);
  }

  void Add(const std::string& desc, uint32 capacity) {
    lock_.WriteLock();
    desc_map_[desc] = capacity;
    lock_.WriteUnlock();
  }
  bool Remove(const std::string& desc) {
    lock_.WriteLock();
    desc_map_.erase(desc);
    lock_.WriteUnlock();
    return true;
  }
  void Clear() {
    lock_.WriteLock();
    desc_map_.clear();
    lock_.WriteUnlock();
  }

  template<typename HashFunc>
  bool Rebuild(HashFunc func) {
    uint32 total_point = 0;

    lock_.ReadLock();
    BOOST_FOREACH(const DescMapType::value_type& i, desc_map_) {
      total_point += i.second;
      // TODO: overflow check
    }
    if(total_point == 0)
    {
      lock_.ReadUnlock();
      return false;
    }

    PointList target;
    // GOD! it's a very huge number
    target.reserve(total_point);

    const size_t big_enough = 16 + 5; // calc from desc_length
    char buf[big_enough];
    // unsigned char digest[16];

    BOOST_FOREACH(const DescMapType::value_type& i, desc_map_) {
      for (uint32 k=0; k<i.second; ++k) {
        int len = snprintf(buf, big_enough, "%s-%x", i.first.c_str(), k);
        uint32 p = func(buf, len);
        target.push_back(Point(p, i.first));
      }
    }

    // TODO : read lock upgrade
    lock_.ReadUnlock();

    // std::sort
    std::sort(target.begin(), target.end(), CompareByPoint());

    lock_.WriteLock();
    points_.swap(target);
    lock_.WriteUnlock();

    return true;
  }

  bool Rebuild() {
    typedef uint32 (*TheHashF)(const char*, size_t);
    return Rebuild<TheHashF>(&Continuum::Hash);
  }

  const size_t size() const {
    lock_.ReadLock();
    int sz = points_.size();
    lock_.ReadUnlock();
    return sz;
  }

  const std::string& name() const {
    return name_;
  }

  std::string Dump() const {
    // TODO: lock
    // 1 count %
    // 2 count %
    std::map<std::string, uint32> distr;
    std::ostringstream oss;
    uint32 before = 0;

    lock_.ReadLock();
    BOOST_FOREACH(const Point& p, points_) {
      distr[p.desc_] += p.point_ - before;
      before = p.point_;
    }

    distr[points_.front().desc_] += (
      std::numeric_limits<uint32>::max() - points_.back().point_
      );

    double fx = 0.0;
    for (std::map<std::string, uint32>::const_iterator i=distr.begin();
      i!=distr.end(); ++i) {
        double d1 = (double)i->second / std::numeric_limits<uint32>::max();
        DescMapType::const_iterator di = desc_map_.find(i->first);
        double d2 = (double)di->second / size();
        double d = d1 - d2;
        fx += d * d;
        oss << i->first << "\t" << i->second << "\t" << 100.0 * i->second / std::numeric_limits<uint32>::max() << " %\n";
    }
    lock_.ReadUnlock();
    oss << "方差 * 100: " << fx * 100 << "\n";
    return oss.str();
  }

  Continuum(const std::string& n = "") : name_(n) {}

private:
  struct Point {
    Point() : point_(0) {}
    Point(uint32 p, const std::string& desc) : point_(p), desc_(desc) {}
    uint32 point_;      // cache point
    std::string desc_;  // 服务器的描述，可以是 ip:port
    // TODO: use char desc[desc_length] if it's better
  };

  struct CompareByPoint {
    bool operator()(const Point& a, const Point& b) const {
      return a.point_ < b.point_; // TODO: right?
    }
  };

  static uint32 HashRaw(const char* text, size_t length) {
    return Hash(text, length);
  }

  mutable ReadWriteLock lock_;

  typedef std::vector<Point> PointList;
  PointList points_;

  // 希望是字典序的列表，故使用map
  typedef std::map<std::string, uint32> DescMapType;
  DescMapType desc_map_;

  std::string name_;
};

} // namespace base
#endif //BASE_CACHE_CLIENT_CONSISTENT_H__
