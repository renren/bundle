#ifndef OCE_BASE_CACHE_H__
#define OCE_BASE_CACHE_H__

#include <list>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include "base3/common.h"
#include "base3/hashmap.h"
#include "base3/ptime.h"
#include "base3/asyncall.h"
#include "base3/ptime.h"
#include "base3/logging.h"

namespace base {

template<typename Key, typename Value, Value* (*Creator)(Key)>
class HashCache {
public:
  typedef std::list<std::pair<Key, Value> > list_type;
  typedef std::hash_map<Key, typename list_type::iterator> map_type;
  typedef Key key_type;

  HashCache(size_t max_size)
    : max_size_(max_size)
    , replacing_(false) {}

  Value Locate(key_type key) {
    {
      boost::mutex::scoped_lock alock(mutex_);

      typename map_type::const_iterator i = map_.find(key);
      if (i != map_.end()) {
        // 最后为最新的条目
        list_.splice(list_.end(), list_, i->second);
        return i->second->second;
      }
    }

    Value* v = Creator(key);
    if (v) {
      Insert(key, *v);
      std::auto_ptr<Value> ap(v); // for leak, ugly
      return *v;
    }

    return Value(); // TODO: return boost::optional<Value>
  }

  Value Find(key_type key) {
    {
      boost::mutex::scoped_lock alock(mutex_);

      typename map_type::const_iterator i = map_.find(key);
      if (i != map_.end()) {
        // 最后为最新的条目
        list_.splice(list_.end(), list_, i->second);
        return i->second->second;
      }
    }

    return Value(); // TODO: return boost::optional<Value>
  }

  void Erase(key_type key) {
    boost::mutex::scoped_lock alock(mutex_);

    typename map_type::iterator i = map_.find(key);
    if (i != map_.end()) {
      list_.erase(i->second);
      map_.erase(i);
    }
  }

  void Insert(key_type key, const Value& v) {
    boost::mutex::scoped_lock alock(mutex_);

    typename list_type::iterator i = list_.insert(
      list_.end(), std::make_pair(key, v)
      );
    map_[key] = i;

    if (!replacing_ && ebaseed()) {
      replacing_ = true;
      Post(boost::bind(&HashCache::Replace, this));
    }
  }

  size_t Replace() {
    PTIME(pt, "Replace", true, 0);
    boost::mutex::scoped_lock alock(mutex_);
    size_t count = map_.size() - max_size_;

    typename list_type::iterator end = list_.begin();
    std::advance(end, count);

    for (typename list_type::iterator i=list_.begin(); i!=end;) {
      map_.erase(i->first);
      i = list_.erase(i);
    }

    replacing_ = false;
    if (count)
      LOG(INFO) << "cache replace " << count;
    return count;
  }

  size_t size() const {
    return map_.size();
  }

  void set_max_size(size_t max_size) {
    max_size_ = max_size;
    if (ebaseed()) {
      replacing_ = true;
      Post(boost::bind(&HashCache::replace, this));
    }
  }

  // 超过限度一定后才开始 replace
  bool ebaseed() const {
    return map_.size() > max_size_ &&
      map_.size() - max_size_ > max_size_/10;
  }

  // for debug
  void Dump(std::ostream& ostem) const {
    boost::mutex::scoped_lock alock(mutex_);
    for (typename list_type::const_iterator i=list_.begin(); i!=list_.end(); ++i) {
      ostem << i->first << " ";
    }
  }

private:
  list_type list_;
  map_type map_;
  size_t max_size_;
  mutable boost::mutex mutex_;

  bool replacing_;
};

}
#endif // OCE_BASE_CACHE_H__
