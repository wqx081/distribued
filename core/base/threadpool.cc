#include "core/base/threadpool.h"
#include "core/system/env.h"

#include <deque>
#include <thread>
#include <vector>

#include <mutex>
#include <condition_variable>

#include "core/base/logging.h"

// IF USE EIGEN_USE_THREADS
#include "core/base/thread/device_thread_pool.h"
#include "core/base/thread/cost_model.h"

#include "core/system/context.h"

namespace mr {
namespace thread {


// IF USE EIGEN_USE_THREADS

struct EigenEnvironment {

  typedef mr::Thread EnvThread;
  struct TaskImpl {
    std::function<void()> f;
    Context context;
    //Not Used Yet
    uint64_t trace_id;
  };
  struct Task {
    std::unique_ptr<TaskImpl> f;
  };
  
  Env* const env_;
  const ThreadOptions thread_options_;
  const string name_;
  
  EigenEnvironment(Env* env, const ThreadOptions& thread_options,
                   const string& name)
    : env_(env), thread_options_(thread_options), name_(name) {}
  
  EnvThread* CreateThread(std::function<void()> f) {
    return env_->StartThread(thread_options_, name_, [=]() {
        //port::ScopedFlushDenormal flush;
      f();
    });
  }

  Task CreateTask(std::function<void()> f) {
    uint64_t id = 0;
#if 0
    if (port::Tracing::IsActive()) {
        id = port::Tracing::UniqueId();
        port::Tracing::RecordEvent(port::Tracing::EventCategory::kScheduleClosure,
                                                                      id);
    }
#endif
      return Task{
          std::unique_ptr<TaskImpl>(new TaskImpl{
              std::move(f), mr::Context(mr::ContextKind::kThread), id,
          }),
      };
    }
  
  void ExecuteTask(const Task& t) {
    WithContext wc(t.f->context);
    if (t.f->trace_id != 0) {
      //port::Tracing::ScopedActivity region(
      //port::Tracing::EventCategory::kRunClosure, t.f->trace_id);
        t.f->f();
      } else {
        t.f->f();
      }
    }
};

struct ThreadPool::Impl : eigen::ThreadPoolTempl<EigenEnvironment> {
    Impl(Env* env, const ThreadOptions& thread_options, const string& name,
                  int num_threads)
        : eigen::ThreadPoolTempl<EigenEnvironment>(
              num_threads, EigenEnvironment(env, thread_options, name)) {}
  
    void ParallelFor(int64_t total, int64_t cost_per_unit,
                     std::function<void(int64_t, int64_t)> fn) {
      CHECK_GE(total, 0);
      CHECK_EQ(total, (int64_t)(eigen::Index)total);
      eigen::ThreadPoolDevice device(this, this->NumThreads());
      device.ParallelFor(total, eigen::OpCost(0, 0, cost_per_unit),
          [&fn](eigen::Index first, eigen::Index last) { fn(first, last); });
  }
};
  

//////////////////////////
//
ThreadPool::ThreadPool(Env* env, const string& name, int num_threads)
      : ThreadPool(env, ThreadOptions(), name, num_threads) {}
      
  ThreadPool::ThreadPool(Env* env, const ThreadOptions& thread_options,
                         const string& name, int num_threads) {
  CHECK_GE(num_threads, 1);
  impl_.reset(
        new ThreadPool::Impl(env, thread_options, "mr_" + name, num_threads));
}     
  
ThreadPool::~ThreadPool() {}
  
void ThreadPool::Schedule(std::function<void()> fn) {
  CHECK(fn != nullptr);
  impl_->Schedule(std::move(fn));
} 
  
void ThreadPool::ParallelFor(int64_t total, int64_t cost_per_unit,
                             std::function<void(int64_t, int64_t)> fn) {
  impl_->ParallelFor(total, cost_per_unit, std::move(fn));
} 
  
int ThreadPool::NumThreads() const { return impl_->NumThreads(); }
  
int ThreadPool::CurrentThreadId() const { return impl_->CurrentThreadId(); }


// IF NOT USE EIGEN_USE_THREADS
#if 0
struct ThreadPool::Impl {
  Impl(Env* env, const ThreadOptions& thread_options, const string& name,
       int num_threads);
  ~Impl();
  void Schedule(std::function<void()> fn);

 private:
  struct Waiter {
    std::condition_variable cv;
    bool ready;
  };

  struct Task {
    std::function<void()> fn;
    uint64_t id;
  };

  void WorkerLoop();

  const string name_;
  std::mutex mu_;
  std::vector<Thread*> threads_;  // All threads
  std::vector<Waiter*> waiters_;  // Stack of waiting threads.
  std::deque<Task> pending_;      // Queue of pending work
};

ThreadPool::Impl::Impl(Env* env, const ThreadOptions& thread_options,
                       const string& name, int num_threads)
    : name_(name) {
  for (int i = 0; i < num_threads; i++) {
    threads_.push_back(
        env->StartThread(thread_options, name, [this]() { WorkerLoop(); }));
  }
}

ThreadPool::Impl::~Impl() {
  {
    // Wait for all work to get done.
    std::lock_guard<std::mutex> l(mu_);

    // Inform every thread to exit.
    for (size_t i = 0; i < threads_.size(); ++i) {
      pending_.push_back({nullptr, 0});
    }

    // Wakeup all waiters.
    for (auto w : waiters_) {
      w->ready = true;
      w->cv.notify_one();
    }
  }

  // Wait for threads to finish.
  for (auto t : threads_) {
    delete t;
  }
}

void ThreadPool::Impl::Schedule(std::function<void()> fn) {
  uint64_t id = 0;

  std::unique_lock<std::mutex> l(mu_);

  pending_.push_back({fn, id});
  if (!waiters_.empty()) {
    Waiter* w = waiters_.back();
    waiters_.pop_back();
    w->ready = true;
    w->cv.notify_one();
  }
}

void ThreadPool::Impl::WorkerLoop() {
  // Set the processor flag to flush denormals to zero
  // port::ScopedFlushDenormal flush;

  std::unique_lock<std::mutex> l(mu_);

  Waiter w;
  while (true) {
    while (pending_.empty()) {
      // Wait for work to be assigned to me
      w.ready = false;
      waiters_.push_back(&w);
      while (!w.ready) {
        w.cv.wait(l);
      }
    }
    // Pick up pending work
    Task t = pending_.front();
    pending_.pop_front();
    if (t.fn == nullptr) {
      break;
    }
    mu_.unlock();
    if (t.id != 0) {
      t.fn();
    } else {
      t.fn();
    }
    mu_.lock();
  }
}

ThreadPool::ThreadPool(Env* env, const string& name, int num_threads)
    : ThreadPool(env, ThreadOptions(), name, num_threads) {}

ThreadPool::ThreadPool(Env* env, const ThreadOptions& thread_options,
                       const string& name, int num_threads) {
  CHECK_GE(num_threads, 1);
  impl_.reset(
      new ThreadPool::Impl(env, thread_options, "tf_" + name, num_threads));
}

ThreadPool::~ThreadPool() {}

void ThreadPool::Schedule(std::function<void()> fn) {
  CHECK(fn != nullptr);
  impl_->Schedule(std::move(fn));
}
#endif

}  // namespace thread
}  // namespace core
