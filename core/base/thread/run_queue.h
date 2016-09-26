#ifndef CORE_BASE_THREAD_RUN_QUEUE_H_
#define CORE_BASE_THREAD_RUN_QUEUE_H_

#include "core/base/macros.h"

#include <atomic>
#include <mutex>

#include <glog/logging.h>

namespace eigen {

template<typename Work, unsigned kSize>
class RunQueue {
 public:
  RunQueue() : front_(0), back_(0) {
    DCHECK((kSize & (kSize - 1)) == 0);    
    DCHECK(kSize > 2);
    DCHECK(kSize <= (64 << 10));
    for (unsigned i = 0; i < kSize; ++i) {
      array_[i].state.store(kEmpty, std::memory_order_relaxed);
    }
  }

  ~RunQueue() {
    DCHECK(Size() == 0);
  }
 
  Work PushFront(Work w) {
    unsigned front = front_.load(std::memory_order_relaxed);
    Element* e = &array_[front & kMask];
    uint8_t s = e->state.load(std::memory_order_relaxed);
    if (s != kEmpty ||
        !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire))
      return w;
    front_.store(front + 1 + (kSize << 1), std::memory_order_relaxed);
    e->w = std::move(w);
    e->state.store(kReady, std::memory_order_release);
    return Work();
  }

  Work PopFront() {
    unsigned front = front_.load(std::memory_order_relaxed);
    Element* e = &array_[(front - 1) & kMask];
    uint8_t s = e->state.load(std::memory_order_relaxed);
    if (s != kReady ||
        !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire))
      return Work();
    Work w = std::move(e->w);
    e->state.store(kEmpty, std::memory_order_release);
    front = ((front - 1) & kMask2) | (front & ~kMask2);
    front_.store(front, std::memory_order_relaxed);
    return w;
  }

  Work PushBack(Work w) {
    std::unique_lock<std::mutex> lock(mutex_);
    unsigned back = back_.load(std::memory_order_relaxed);
    Element* e = &array_[(back - 1) & kMask];
    uint8_t s = e->state.load(std::memory_order_relaxed);
    if (s != kEmpty ||
        !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire))
      return w;
    back = ((back - 1) & kMask2) | (back & ~kMask2);
    back_.store(back, std::memory_order_relaxed);
    e->w = std::move(w);
    e->state.store(kReady, std::memory_order_release);
    return Work();
  }

  Work PopBack() {
    if (Empty()) return Work();
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (!lock) return Work();
    unsigned back = back_.load(std::memory_order_relaxed);
    Element* e = &array_[back & kMask];
    uint8_t s = e->state.load(std::memory_order_relaxed);
    if (s != kReady ||
        !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire))
      return Work();
    Work w = std::move(e->w);
    e->state.store(kEmpty, std::memory_order_release);
    back_.store(back + 1 + (kSize << 1), std::memory_order_relaxed);
    return w;
  }

  unsigned PopBackHalf(std::vector<Work>* result) {
    if (Empty()) return 0;
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (!lock) return 0;
    unsigned back = back_.load(std::memory_order_relaxed);
    unsigned size = Size();
    unsigned mid = back;
    if (size > 1) mid = back + (size - 1) / 2;
    unsigned n = 0;
    unsigned start = 0;
    for (; static_cast<int>(mid - back) >= 0; mid--) {
      Element* e = &array_[mid & kMask];
      uint8_t s = e->state.load(std::memory_order_relaxed);
      if (n == 0) {
        if (s != kReady ||
            !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire))
            continue;
        start = mid;
      } else {
        DCHECK(s == kReady);
      } 
      result->push_back(std::move(e->w));
      e->state.store(kEmpty, std::memory_order_release);
      n++;
    } 
    if (n != 0)
      back_.store(start + 1 + (kSize << 1), std::memory_order_relaxed);
    return n;
  } 

  unsigned Size() const {
    for (;;) {
      unsigned front = front_.load(std::memory_order_acquire);
      unsigned back = back_.load(std::memory_order_acquire);
      unsigned front1 = front_.load(std::memory_order_relaxed);
      if (front != front1) continue;
      int size = (front & kMask2) - (back & kMask2);
      if (size < 0) size += 2 * kSize;
      if (size > static_cast<int>(kSize)) size = kSize;
      return size;
    } 
  } 
    
  bool Empty() const { return Size() == 0; }
    


 private:
  static const unsigned kMask = kSize -1;
  static const unsigned kMask2= (kSize << 1) - 1;

  struct Element {
    std::atomic<uint8_t> state;
    Work w;
  };

  enum {
    kEmpty,
    kBusy,
    kReady,
  };
  std::mutex mutex_;

  std::atomic<unsigned> front_;
  std::atomic<unsigned> back_;
  Element array_[kSize];

  DISALLOW_COPY_AND_ASSIGN(RunQueue);
};

} // namespace core
#endif // CORE_BASE_THREAD_RUN_QUEUE_H_
