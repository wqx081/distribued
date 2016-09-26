#ifndef CORE_BASE_THREAD_POOL_INTERFACE_H_
#define CORE_BASE_THREAD_POOL_INTERFACE_H_

#include <functional>

namespace eigen {

class ThreadPoolInterface {
 public:
  virtual ~ThreadPoolInterface() {}

  virtual void Schedule(std::function<void()> fn) = 0;
  virtual int NumThreads() const = 0;
  virtual int CurrentThreadId() const = 0;

};

} // namespace eigen
#endif // CORE_BASE_THREAD_POOL_INTERFACE_H_
