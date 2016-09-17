#include "core/base/threadpool.h"

#include <deque>
#include <thread>
#include <vector>

#include <mutex>
#include <condition_variable>

#include "core/base/logging.h"

namespace mr {
namespace thread {

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

}  // namespace thread
}  // namespace tensorflow
