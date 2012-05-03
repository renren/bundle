字符串操作类
 stringencode.h stringdigest.h md5.h

异步、线程池
 1 工作线程拥有自己的队列的线程池实现
 threadpool.h messagequeue.h thread.h
 优点：吞吐量还是很高的
 TODO: 支持直接加入 boost::function
  使用 Google-Log?
 
 2 asyncall.h
 自动增长的异步线程池，boost::function heavy used
  TODO: 使用 baseasync.h 写法实现
 
 3 baseasync.h
 **只**能在内部(base)使用的异步线程池，现在只有**1**个线程
 
 4 rwlock.h 玉伟实现的写优先的读写锁，有测试用例
  TODO: 加入 spinlock, atomic from google-perftools，如何加入是个问题
 
LOGGING
 logging.h/cc time_.h/cc logrotate.h
 时间有点怪,需要换么?

辅助
 basictypes.h 类型定义，对cpu字长敏感的代码，***直接***使用带长度的类型
 common.h 调试支持，内部线程池等
 hashmap.h 统一多编译器下hash_map的写法 std::hash_map<k, v>
  TODO: use TR1's unordered_map

PTIME

PCOUNT

hash.h
 经过测试(代码位于cache/)后，分布、性能均为最优的hash函数
 TODO: 实现 int64 版本
 TODO: 把doobs_hash扔到cache下
 
jdbcurl.h
 **一定**要推广使用这种方式的数据库连接形式

cache.h
 模仿 oce::ObjectCache 的轻量级实现




scoped_ptr.h
scoped_ptr<T>.release() 是返回保存的指针
