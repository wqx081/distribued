#ifndef FRAMEWORK_DEVICE_BASE_H_
#define FRAMEWORK_DEVICE_BASE_H_
#include <memory>
#include <unordered_map>

#include "protobuf/device_attributes.pb.h"
#include "core/base/refcount.h"
#include "core/base/status.h"
#include "core/base/logging.h"

namespace mr {
class Env;

namespace thread {
class ThreadPool;
} // namespace thread
} // namespace mr

namespace mr {

	
class DeviceBase {
 public:
  explicit DeviceBase(Env* env) : env_(env) {}
  virtual ~DeviceBase();

  Env* env() const { return env_; }

  struct CpuWorkerThreads {
    int num_threads = 0;
    thread::ThreadPool* workers = nullptr;
  };

  void set_cpu_worker_threads(CpuWorkerThreads* t) {
    cpu_worker_threads_ = t;
  }

  const CpuWorkerThreads* cpu_worker_threads() const {
    CHECK(cpu_worker_threads_ != nullptr);
    return cpu_worker_threads_;
  }

  virtual const DeviceAttributes& attributes() const {
    LOG(FATAL) << "Device does not implement attributes()";
  }

 private:
  Env* const env_;
  CpuWorkerThreads* cpu_worker_threads_ = nullptr;
};

} // namespace mr

#endif //FRAMEWORK_DEVICE_BASE_H_
