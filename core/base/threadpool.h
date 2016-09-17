#ifndef TENSORFLOW_LIB_CORE_THREADPOOL_H_
#define TENSORFLOW_LIB_CORE_THREADPOOL_H_

#include <functional>
#include <memory>
#include "core/system/env.h"
#include "core/base/macros.h"

namespace mr {
namespace thread {

class ThreadPool {
 public:
  ThreadPool(Env* env, const string& name, int num_threads);

  ThreadPool(Env* env, const ThreadOptions& thread_options, const string& name,
             int num_threads);

  ~ThreadPool();

  void Schedule(std::function<void()> fn);

  struct Impl;

 private:
  std::unique_ptr<Impl> impl_;
  DISALLOW_COPY_AND_ASSIGN(ThreadPool);
};

}  // namespace thread
}  // namespace tensorflow

#endif  // TENSORFLOW_LIB_CORE_THREADPOOL_H_
