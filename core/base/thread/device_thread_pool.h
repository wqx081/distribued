#ifndef CORE_BASE_THREAD_DEVICE_THREAD_POOL_H_
#define CORE_BASE_THREAD_DEVICE_THREAD_POOL_H_

#include "core/base/thread/simple_thread_pool.h"
#include "core/base/thread/non_blocking_thread_pool.h"
#include "core/base/thread/cost_model.h"

#include <algorithm>

namespace eigen {


typedef int Index;

namespace {

// ! USE_SIMPLE_THREAD_POOL
template<typename Env> using ThreadPoolTempl = NonBlockingThreadPoolTempl<Env>;
typedef NonBlockingThreadPool ThreadPool;
// ELSE
// template<typename Env> using ThreadPoolTempl = SimpleThreadPoolTempl<Env>;
// typedef SimpleThreadPool ThreadPool;

template <typename T, typename X, typename Y>
inline
T divup(const X x, const Y y) {
    return static_cast<T>((x + y - 1) / y);
}
  
template <typename T>
inline
T divup(const T x, const T y) {
  return static_cast<T>((x + y - 1) / y);
}

} // namespace

class Barrier {
 public:
  Barrier(unsigned int count) : state_(count << 1), notified_(false) {
    DCHECK(((count << 1) >> 1) == count);
  }
  ~Barrier() {
    DCHECK((state_>>1) == 0);
  }

  void Notify() {
    unsigned int v = state_.fetch_sub(2, std::memory_order_acq_rel) - 2;
    if (v != 1) {
      DCHECK(((v + 2) & ~1) != 0);
      return;  // either count has not dropped to 0, or waiter is not waiting
    }
    std::unique_lock<std::mutex> l(mu_);
    DCHECK(!notified_);
    notified_ = true;
    cv_.notify_all();
  }

  void Wait() {
    unsigned int v = state_.fetch_or(1, std::memory_order_acq_rel);
    if ((v >> 1) == 0) return;
    std::unique_lock<std::mutex> l(mu_);
    while (!notified_) {
      cv_.wait(l);
    }
  }

 private:
  std::mutex mu_;
  std::condition_variable cv_;
  std::atomic<unsigned int> state_;  // low bit is waiter flag
  bool notified_;
};

struct Notification : Barrier {
  Notification() : Barrier(1) {};
};


template <typename Function, typename... Args> struct FunctionWrapperWithNotification
{
  static void run(Notification* n, Function f, Args... args) {
    f(args...);
    if (n) {
      n->Notify();
    } 
  } 
};

template <typename Function, typename... Args> struct FunctionWrapperWithBarrier
{
  static void run(Barrier* b, Function f, Args... args) {
    f(args...);
    if (b) {
      b->Notify();
    } 
  } 
};

template <typename SyncType>
static inline void wait_until_ready(SyncType* n) {
  if (n) {
    n->Wait();
  } 
} 

//////////////
class ThreadPoolDevice {
 public:
  ThreadPoolDevice(ThreadPoolInterface* pool, int num_cores) 
    : pool_(pool), num_threads_(num_cores) { }  

  int NumThreads() const {
    return num_threads_;
  }

  template <class Function, class... Args>
  Notification* enqueue(Function&& f, Args&&... args) const {
    Notification* n = new Notification();
    pool_->Schedule(std::bind(&FunctionWrapperWithNotification<Function, Args...>::run, 
                              n, 
                              f, 
                              args...));
    return n;
  }

  template <class Function, class... Args>
  void enqueue_with_barrier(Barrier* b,
                            Function&& f,       
                            Args&&... args) const {
    pool_->Schedule(std::bind(
        &FunctionWrapperWithBarrier<Function, Args...>::run, b, f, args...));
  }

  template <class Function, class... Args>
  void enqueueNoNotification(Function&& f, Args&&... args) const {
    pool_->Schedule(std::bind(f, args...));
  }

  int CurrentThreadId() const {
    return pool_->CurrentThreadId();
  } 

  //TODO
  
  void ParallelFor(Index n,
                   const OpCost& cost,
                   std::function<Index(Index)> block_align,
                   std::function<void(Index, Index)> f) const {

    typedef CostModel<ThreadPoolDevice> CostModel;
    if (n <= 1 || NumThreads() == 1 ||
        CostModel::numThreads(n, cost, static_cast<int>(NumThreads())) == 1) {
      f(0, n);
      return;
    }

   double block_size_f = 1.0 / CostModel::taskSize(1, cost);
    Index block_size = std::min(n, std::max<Index>(1, block_size_f));
    const Index max_block_size =
        std::min(n, std::max<Index>(1, 2 * block_size_f));
    if (block_align) {
      Index new_block_size = block_align(block_size);
      DCHECK(new_block_size >= block_size);
      block_size = std::min(n, new_block_size);
    }
    Index block_count = divup(n, block_size);
    double max_efficiency =
        static_cast<double>(block_count) /
        (divup<int>(block_count, NumThreads()) * NumThreads());
    for (Index prev_block_count = block_count; prev_block_count > 1;) {
      Index coarser_block_size = divup(n, prev_block_count - 1);
      if (block_align) {
        Index new_block_size = block_align(coarser_block_size);
        DCHECK(new_block_size >= coarser_block_size);
        coarser_block_size = std::min(n, new_block_size);
      } 
      if (coarser_block_size > max_block_size) {
        break;  // Reached max block size. Stop.
      } 
      const Index coarser_block_count = divup(n, coarser_block_size);
      DCHECK(coarser_block_count < prev_block_count);
      prev_block_count = coarser_block_count;
      const double coarser_efficiency =
          static_cast<double>(coarser_block_count) /
          (divup<int>(coarser_block_count, NumThreads()) * NumThreads());
      if (coarser_efficiency + 0.01 >= max_efficiency) { 
        block_size = coarser_block_size;
        block_count = coarser_block_count;
        if (max_efficiency < coarser_efficiency) {
          max_efficiency = coarser_efficiency;
        } 
      } 
    } 
    Barrier barrier(static_cast<unsigned int>(block_count));
    std::function<void(Index, Index)> handleRange;
    handleRange = [=, &handleRange, &barrier, &f](Index first, Index last) {
      if (last - first <= block_size) {
        f(first, last);
        barrier.Notify();
        return;
      }
      Index mid = first + divup((last - first) / 2, block_size) * block_size;
      pool_->Schedule([=, &handleRange]() { handleRange(mid, last); });
      pool_->Schedule([=, &handleRange]() { handleRange(first, mid); });
    };
    handleRange(0, n);
    barrier.Wait();
  }

  void ParallelFor(Index n, const OpCost& cost,
                   std::function<void(Index, Index)> f) const {
    ParallelFor(n, cost, nullptr, std::move(f));
  }

 private:
  ThreadPoolInterface* pool_;
  int num_threads_;
};


} // namespace core
#endif // CORE_BASE_THREAD_DEVICE_THREAD_POOL_H_
