#include "core/base/threadpool.h"

#include <atomic>
#include <mutex>

#include "core/system/env.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace mr {
namespace thread {

static const int kNumThreads = 30;

TEST(ThreadPool, Empty) {
  for (int num_threads = 1; num_threads < kNumThreads; num_threads++) {
   LOG(INFO) <<  "Testing with %d " << num_threads << " threads";
   ThreadPool pool(Env::Default(), "test", num_threads);
  }
}

TEST(ThreadPool, DoWork) {
  for (int num_threads = 1; num_threads < kNumThreads; ++num_threads) {
    LOG(INFO) << "Testing with " << num_threads << " threads";
    const int kWorkItems = 15;
    bool work[kWorkItems];
    for (int i = 0; i < kWorkItems; ++i) {
      work[i] = false;
    }
    {
      ThreadPool pool(Env::Default(), "test", num_threads);
      for (int i = 0; i < kWorkItems; ++i) {
        pool.Schedule([&work, i]() {
          ASSERT_FALSE(work[i]);
          work[i] = true;
        });
      }
    }
    for (int i = 0; i < kWorkItems; ++i) {
      ASSERT_TRUE(work[i]);
    }
  }
}

} // namespace thread

} // namespace mr
