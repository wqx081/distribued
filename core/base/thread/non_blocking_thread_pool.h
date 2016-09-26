#ifndef CORE_BASE_THREAD_NON_BLOCKING_THREAD_POOL_H_
#define CORE_BASE_THREAD_NON_BLOCKING_THREAD_POOL_H_
#include "core/base/thread/thread_pool_interface.h"
#include "core/base/thread/thread_environment.h"
#include "core/base/thread/event_count.h"
#include "core/base/thread/run_queue.h"
#include "core/base/thread/max_size_vector.h"

namespace eigen {

template <typename Environment>
class NonBlockingThreadPoolTempl : public ThreadPoolInterface {
 public:
  typedef typename Environment::Task Task;
  typedef RunQueue<Task, 1024> Queue;

    NonBlockingThreadPoolTempl(int num_threads, Environment env = Environment())
        : env_(env),
          threads_(num_threads),
          queues_(num_threads),
          coprimes_(num_threads),
          waiters_(num_threads),
          blocked_(0),
          spinning_(0),
          done_(false),
          ec_(waiters_) {
      waiters_.resize(num_threads);
  
      for (int i = 1; i <= num_threads; i++) {
        unsigned a = i; 
        unsigned b = num_threads;
        while (b != 0) {
          unsigned tmp = a;
          a = b;
          b = tmp % b;
        } 
        if (a == 1) {
          coprimes_.push_back(i);
        } 
      } 
      for (int i = 0; i < num_threads; i++) {
        queues_.push_back(new Queue());
      } 
      for (int i = 0; i < num_threads; i++) {
        threads_.push_back(env_.CreateThread([this, i]() { WorkerLoop(i); }));
      } 
    } 

    
    ~NonBlockingThreadPoolTempl() {
      done_ = true;
  
      ec_.Notify(true);
      
      for (size_t i = 0; i < threads_.size(); i++) delete threads_[i];
      for (size_t i = 0; i < threads_.size(); i++) delete queues_[i];
    } 
    
void Schedule(std::function<void()> fn) {
      Task t = env_.CreateTask(std::move(fn));
      PerThread* pt = GetPerThread();
      if (pt->pool == this) {
        Queue* q = queues_[pt->thread_id];
        t = q->PushFront(std::move(t));
      } else {
        Queue* q = queues_[Rand(&pt->rand) % queues_.size()];
        t = q->PushBack(std::move(t));
      } 
      if (!t.f)
        ec_.Notify(false);
      else
        env_.ExecuteTask(t);  // Push failed, execute directly.
    }   
      
    int NumThreads() const final {
      return static_cast<int>(threads_.size());
    }

    int CurrentThreadId() const final {
      const PerThread* pt =
          const_cast<NonBlockingThreadPoolTempl*>(this)->GetPerThread();
if (pt->pool == this) {
        return pt->thread_id;
      } else {
        return -1; 
      } 
    } 
        
 private:
  typedef typename Environment::EnvThread Thread;
  
  struct PerThread {
    constexpr PerThread() : pool(NULL), rand(0), thread_id(-1) { }
    NonBlockingThreadPoolTempl* pool;  // Parent pool, or null for normal threads.
    uint64_t rand;  // Random generator state.
    int thread_id;  // Worker thread index in pool.
  };
  
  Environment env_;
  MaxSizeVector<Thread*> threads_;
  MaxSizeVector<Queue*> queues_;
  MaxSizeVector<unsigned> coprimes_;
  MaxSizeVector<EventCount::Waiter> waiters_;
  std::atomic<unsigned> blocked_;
  std::atomic<bool> spinning_;
  std::atomic<bool> done_;
  EventCount ec_;

  void WorkerLoop(int thread_id) {
      PerThread* pt = GetPerThread();
      pt->pool = this;
      pt->rand = std::hash<std::thread::id>()(std::this_thread::get_id());
      pt->thread_id = thread_id;
      Queue* q = queues_[thread_id];
      EventCount::Waiter* waiter = &waiters_[thread_id];
      for (;;) {
        Task t = q->PopFront();
        if (!t.f) {
          t = Steal();
          if (!t.f) {
            if (!spinning_ && !spinning_.exchange(true)) {
              for (int i = 0; i < 1000 && !t.f; i++) {
                t = Steal();
              } 
              spinning_ = false;
            } 
            if (!t.f) {
              if (!WaitForWork(waiter, &t)) {
                return;
              } 
            } 
          } 
        } 
        if (t.f) {
          env_.ExecuteTask(t);
        } 
      } 
  } 

      Task Steal() {
      PerThread* pt = GetPerThread();
      const size_t size = queues_.size();
      unsigned r = Rand(&pt->rand);
      unsigned inc = coprimes_[r % coprimes_.size()];
      unsigned victim = r % size;
      for (unsigned i = 0; i < size; i++) {
        Task t = queues_[victim]->PopBack();
        if (t.f) {
          return t;
        } 
        victim += inc;
        if (victim >= size) {
          victim -= size;
        }
      }
      return Task();
    }

    bool WaitForWork(EventCount::Waiter* waiter, Task* t) {
      DCHECK(!t->f);
      ec_.Prewait(waiter);
      int victim = NonEmptyQueueIndex();
      if (victim != -1) {
        ec_.CancelWait(waiter);
        *t = queues_[victim]->PopBack();
        return true;
      } 
      blocked_++;
      if (done_ && blocked_ == threads_.size()) {
        ec_.CancelWait(waiter);
        if (NonEmptyQueueIndex() != -1) {
          blocked_--;
          return true;
        } 
        ec_.Notify(true);
        return false;
      } 
      ec_.CommitWait(waiter);
      blocked_--;
      return true;
    } 
      
    int NonEmptyQueueIndex() {
      PerThread* pt = GetPerThread();
      const size_t size = queues_.size();
      unsigned r = Rand(&pt->rand);
      unsigned inc = coprimes_[r % coprimes_.size()];
      unsigned victim = r % size;
      for (unsigned i = 0; i < size; i++) {
        if (!queues_[victim]->Empty()) {
          return victim;
        } 
        victim += inc;
        if (victim >= size) {
          victim -= size;
        }
      } 
      return -1; 
    }   

    static inline PerThread* GetPerThread() {
      static thread_local PerThread per_thread_;
      PerThread* pt = &per_thread_;
      return pt;
    }
  
    static inline unsigned Rand(uint64_t* state) {
      uint64_t current = *state;
      *state = current * 6364136223846793005ULL + 0xda3e39cb94b95bdbULL;
      return static_cast<unsigned>((current ^ (current >> 22)) >> (22 + (current >> 61)));
    }

};

typedef NonBlockingThreadPoolTempl<StlThreadEnvironment> NonBlockingThreadPool;

} // namespace
#endif // CORE_BASE_THREAD_NON_BLOCKING_THREAD_POOL_H_
