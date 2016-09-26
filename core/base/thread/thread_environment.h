#ifndef CORE_BASE_THREAD_ENVIRONMENT_H_
#define CORE_BASE_THREAD_ENVIRONMENT_H_

#include <functional>
#include <thread>

namespace eigen {

struct StlThreadEnvironment {
  struct Task {
    std::function<void()> f;
  };

  class EnvThread {
   public:
    EnvThread(std::function<void()> f) : thr_(std::move(f)) {}
    ~EnvThread() { thr_.join(); }

   private:
    std::thread thr_;
  };

  EnvThread* CreateThread(std::function<void()> f) {
    return new EnvThread(std::move(f));
  }

  Task CreateTask(std::function<void()> f) {
    return Task{std::move(f)};
  }

  void ExecuteTask(const Task& t) {
    t.f();
  }

};

} // namespace core
#endif // CORE_BASE_THREAD_ENVIRONMENT_H_
