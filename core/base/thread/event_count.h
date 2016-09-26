#ifndef CORE_BASE_THREAD_EVENT_COUNT_H_
#define CORE_BASE_THREAD_EVENT_COUNT_H_

#include "core/base/macros.h"
#include "core/base/thread/max_size_vector.h"

#include <vector>
#include <atomic>
#include <thread>
#include <condition_variable>

#include <glog/logging.h>

namespace eigen {

class EventCount {
 public:
  class Waiter;

  EventCount(MaxSizeVector<Waiter>& waiters) : waiters_(waiters) {
      DCHECK(waiters.size() < (1 << kWaiterBits) - 1);
      state_ = kStackMask | (kEpochMask - kEpochInc * waiters.size() * 2);
    }
  
    ~EventCount() {
      DCHECK((state_.load() & (kStackMask | kWaiterMask)) == kStackMask);
    }
  
    void Prewait(Waiter* w) {
      w->epoch = state_.fetch_add(kWaiterInc, std::memory_order_relaxed);
      std::atomic_thread_fence(std::memory_order_seq_cst);
    } 
    
    void CommitWait(Waiter* w) {
      w->state = Waiter::kNotSignaled;
      uint64_t epoch =
          (w->epoch & kEpochMask) +
          (((w->epoch & kWaiterMask) >> kWaiterShift) << kEpochShift);
      uint64_t state = state_.load(std::memory_order_seq_cst);
      for (;;) {
        if (int64_t((state & kEpochMask) - epoch) < 0) {
          std::this_thread::yield();
          state = state_.load(std::memory_order_seq_cst);
          continue;
        } 
        if (int64_t((state & kEpochMask) - epoch) > 0) return;
        DCHECK((state & kWaiterMask) != 0);
        uint64_t newstate = state - kWaiterInc + kEpochInc;
        newstate = (newstate & ~kStackMask) | (w - &waiters_[0]);
        if ((state & kStackMask) == kStackMask)
          w->next.store(nullptr, std::memory_order_relaxed);
        else
          w->next.store(&waiters_[state & kStackMask], std::memory_order_relaxed);
        if (state_.compare_exchange_weak(state, newstate,
                                         std::memory_order_release))
          break;
      }
      Park(w);
    }

    void CancelWait(Waiter* w) {
      uint64_t epoch =
          (w->epoch & kEpochMask) +
          (((w->epoch & kWaiterMask) >> kWaiterShift) << kEpochShift);
      uint64_t state = state_.load(std::memory_order_relaxed);
      for (;;) {
        if (int64_t((state & kEpochMask) - epoch) < 0) {
std::this_thread::yield();
          state = state_.load(std::memory_order_relaxed);
          continue;
        } 
        if (int64_t((state & kEpochMask) - epoch) > 0) return;
        DCHECK((state & kWaiterMask) != 0);
        if (state_.compare_exchange_weak(state, state - kWaiterInc + kEpochInc,
                                         std::memory_order_relaxed)) 
          return;                        
      }   
    } 

    void Notify(bool all) {
      std::atomic_thread_fence(std::memory_order_seq_cst);
      uint64_t state = state_.load(std::memory_order_acquire);
      for (;;) {
        if ((state & kStackMask) == kStackMask && (state & kWaiterMask) == 0)
          return;
        uint64_t waiters = (state & kWaiterMask) >> kWaiterShift;
        uint64_t newstate; 
        if (all) {
          newstate = (state & kEpochMask) + (kEpochInc * waiters) + kStackMask;
        } else if (waiters) { 
                                       newstate = state + kEpochInc - kWaiterInc;
                                     } else {
          Waiter* w = &waiters_[state & kStackMask];
          Waiter* wnext = w->next.load(std::memory_order_relaxed);
          uint64_t next = kStackMask;
          if (wnext != nullptr) next = wnext - &waiters_[0];
          newstate = (state & kEpochMask) + next;
        } 
        if (state_.compare_exchange_weak(state, newstate,
                                         std::memory_order_acquire)) {
          if (!all && waiters) return;  // unblocked pre-wait thread
          if ((state & kStackMask) == kStackMask) return;
          Waiter* w = &waiters_[state & kStackMask];
          if (!all) w->next.store(nullptr, std::memory_order_relaxed);
          Unpark(w);
          return;
        }
      }
    }

  class Waiter {
    friend class EventCount;
    __attribute__((aligned(128))) std::atomic<Waiter*> next;
    std::mutex mu;
    std::condition_variable cv;
    uint64_t epoch;
    unsigned state;
    enum {
      kNotSignaled,
      kWaiting,
      kSignaled,
    };
  };

 private:
  static const uint64_t kStackBits = 16;
  static const uint64_t kStackMask = (1ull << kStackBits) - 1;
  static const uint64_t kWaiterBits = 16;
  static const uint64_t kWaiterShift = 16;
  static const uint64_t kWaiterMask = ((1ull << kWaiterBits) - 1)
                                        << kWaiterShift;
  static const uint64_t kWaiterInc = 1ull << kWaiterBits;
  static const uint64_t kEpochBits = 32;
  static const uint64_t kEpochShift = 32;
  static const uint64_t kEpochMask = ((1ull << kEpochBits) - 1) << kEpochShift;
  static const uint64_t kEpochInc = 1ull << kEpochShift;
  std::atomic<uint64_t> state_;
  MaxSizeVector<Waiter>& waiters_;

  void Park(Waiter* w) {
    std::unique_lock<std::mutex> lock(w->mu);
    while (w->state != Waiter::kSignaled) {
      w->state = Waiter::kWaiting;
      w->cv.wait(lock);
    }
  }
  
  void Unpark(Waiter* waiters) {
    Waiter* next = nullptr;
    for (Waiter* w = waiters; w; w = next) {
      next = w->next.load(std::memory_order_relaxed);
      unsigned state;
      {
        std::unique_lock<std::mutex> lock(w->mu);
        state = w->state;
        w->state = Waiter::kSignaled;
      }
      if (state == Waiter::kWaiting) w->cv.notify_one();
    }
  }

  DISALLOW_COPY_AND_ASSIGN(EventCount);
};

} // namespace core
#endif // CORE_BASE_THREAD_EVENT_COUNT_H_
