#ifndef BASE_BASE_THREAD_H__
#define BASE_BASE_THREAD_H__

// boost::thread 不能设置 stack size, Hack here

#include <memory>
#include <boost/throw_exception.hpp>
#include <pthread.h>

namespace base {

extern "C" void* base_detail_posix_thread_function(void* arg);  

class thread {
public:
   template <typename Function>
   thread(Function f, size_t stack_size)
      : joined_(false) 
  {
    start_thread(f, stack_size);
  }

  ~thread() {
    if (!joined_)
      ::pthread_detach(thread_);
  }

  bool joinable() const {
	return true; // TODO:
  }

  // Wait for the thread to exit.
  void join() {
    if (!joined_) {
      ::pthread_join(thread_, 0);
      joined_ = true;
    }
  }
private:
   thread(thread&);
   thread& operator=(thread&);

private:
  friend void* base_detail_posix_thread_function(void* arg);

  template <typename Function>
  int start_thread(Function f, size_t stack_size); 

  class func_base {
  public:
    virtual ~func_base() {}
    virtual void run() = 0;
  };

  template <typename Function>
  class func : public func_base   {
  public:
    func(Function f)
      : f_(f)
    {}

    virtual void run() {
      f_();
    }

  private:
    Function f_;
  };

  ::pthread_t thread_;
  bool joined_;
};

inline void* base_detail_posix_thread_function(void* arg)
{
  std::auto_ptr<thread::func_base> f(
      static_cast<thread::func_base*>(arg)
      );
  f->run();
  return 0;
}

template<typename Function>
int thread::start_thread(Function f, size_t stack_size) {
#if 0
  线程堆栈对内存开销非常大，不可忽视
  https://computing.llnl.gov/tutorials/pthreads/
  Node Architecture 	#CPUs 	Memory (GB) 	Default Size
    (bytes)
   AMD Opteron 	8 	16 	2,097,152
    Intel IA64 	4 	8 	33,554,432
    Intel IA32 	2 	4 	2,097,152
    IBM Power5 	8 	32 	196,608
    IBM Power4 	8 	16 	196,608
    IBM Power3 	16 	16 	98,304
    IA32        1   2   8,388,608 <-- my ubuntu
#endif
	
	pthread_attr_t attr;
  pthread_attr_init(&attr);
  int error = pthread_attr_setstacksize(&attr, stack_size);
  if (0 != error)
	  return error;
  pthread_attr_destroy(&attr);

  std::auto_ptr<func_base> arg(new func<Function>(f));
  error = ::pthread_create(&thread_, 0,
          base_detail_posix_thread_function, arg.get());
  if (0 != error)
  	return error;

   arg.release();
   return 0;
}
  

}

#endif // BASE_BASE_THREAD_H__
