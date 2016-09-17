#ifndef DR_CALL_OPTIONS_H_
#define DR_CALL_OPTIONS_H_

#include <functional>

#include "core/base/macros.h"
#include <mutex>

namespace mr {

class CallOptions {
 public:
  CallOptions();

  void StartCancel();
  typedef std::function<void()> CancelFunction;
  void SetCancelCallback(CancelFunction cancel_func);
  void ClearCancelCallback();

  int64_t GetTimeout();
  void SetTimeout(int64_t ms);

 private:
  std::mutex mu_;
  CancelFunction cancel_func_;

  // RPC operation timeout
  int64_t timeout_in_ms_;

  DISALLOW_COPY_AND_ASSIGN(CallOptions);
};

}
#endif // DR_CALL_OPTIONS_H_
