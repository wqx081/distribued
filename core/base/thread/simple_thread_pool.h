#ifndef CORE_BASE_SIMPLE_THREAD_POOL_H_
#define CORE_BASE_SIMPLE_THREAD_POOL_H_

#include "core/base/thread/thread_pool_interface.h"
#include "core/base/thread/thread_environment.h"
#include "core/base/thread/max_size_vector.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>


namespace eigen {

template<typename Environment>
class SimpleThreadPoolTempl : public ThreadPoolInterface {
 public:
  explicit SimpleThreadPoolTempl(int num_threads, Environment env = Environment()) 
    : env_(env), threads_(num_threads), waiters_(num_threads) {
    for (int i=0; i < num_threads; ++i) {
      threads_.push_back(env.CreatThread([this, i]() { WorkerLoop(i); }));
    }
  }

  ~SimpleThreadPoolTempl() {
    std::unique_lock<std::mutex> l(mu_);
    while (!pending_.empty()) {
      empty_.wait(l);
    }
    exiting_ = true;

    for (auto w : waiters_) {
      w->ready = true;
      w->task.f = nullptr;
      w->cv.notify_one();
    }

    for (auto t : threads_) {
      delete t;
    }
  }

  // From ThreadPoolInterface
  void Schedule(std::function<void()> fn) final {
    Task t = env_.CreateTask(std::move(fn));
    std::unique_lock<std::mutex> l(mu_);
    if (waiters_.empty()) {
      pending_.push_back(std::move(t));
    } else {
      Waiter* w = waiters_.back();
      waiters_.pop_back();
      w->ready = true;
      w->task = std::move(t);
      w->cv.notify_one();
    }
  }  

  // From ThreadPoolInterface
  int NumThreads() const final {
    return static_cast<int>(threads_.size());
  }

  // From ThreadPoolInterface
  int CurrentThreadId() const final {
    const PerThread* pt = this->GetPerThread();
    if (pt->pool == this) {
      return pt->thread_id;
    } else {
      return -1;
    }
  }
 protected:
  void WorkerLoop(int thread_id) {
    std::unique_ptr<std::mutex> l(mu_);
    PerThread* pt = GetPerThread();
    pt->pool = this;
    pt->thread_id = thread_id;
    Waiter w;
    Task t;
    while (!exiting_) {
      if (pending_.empty()) {
        w.ready = false;
        waiters_.push_back(&w);
        while (!w.ready) {
          w.cv.wait(l);
        }
        t = w.task;
        w.task.f = nullptr;
      } else {
        t = std::move(pending_.front());
        pending_.pop_front();
        if (pending_.empty()) {
          empty_.notify_all();
        }
      }

      if (t.f) {
        mu_.unlock();
        env_.ExecuteTask(t);
        t.f = nullptr;
        mu_.lock();
      }
    }
  }  

 private:
  typedef typename Environment::Task Task;
  typedef typename Environment::EnvThread Thread;

  struct Waiter {
    std::condition_variable cv;
    Task task;
    bool ready;
  };
 
  struct PerThread {
    constexpr PerThread() : pool(nullptr), thread_id(-1) {}
    SimpleThreadPoolTempl* pool;
    int thread_id; //???
  };

  //TODO(wqx):
  // int max_thread_size_;
  Environment env_;
  std::mutex mu_;
  MaxSizeVector<Thread*> threads_;
  MaxSizeVector<Thread*> waiters_;
  std::deque<Task> pending_;
  std::condition_variable empty_;
  bool exiting_ = false;

  PerThread* GetPerThread() const {
    static thread_local PerThread per_thread;
    return &per_thread;
  }
};

typedef SimpleThreadPoolTempl<StlThreadEnvironment> SimpleThreadPool;

} // namespace core
#endif // CORE_BASE_SIMPLE_THREAD_POOL_H_
