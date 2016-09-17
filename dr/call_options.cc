#include "dr/call_options.h"

namespace mr {


CallOptions::CallOptions() {}

void CallOptions::StartCancel() {
  std::unique_lock<std::mutex> l(mu_);
  if (cancel_func_ != nullptr) {
    cancel_func_();
  }
}

void CallOptions::SetCancelCallback(CancelFunction cancel_func) {
  std::unique_lock<std::mutex> l(mu_);
  cancel_func_ = std::move(cancel_func);
}

void CallOptions::ClearCancelCallback() {
  std::unique_lock<std::mutex> l(mu_);
  cancel_func_ = nullptr;
}

int64_t CallOptions::GetTimeout() {
  std::unique_lock<std::mutex> l(mu_);
  return timeout_in_ms_;
}

void CallOptions::SetTimeout(int64_t ms) {
  std::unique_lock<std::mutex> l(mu_);
  timeout_in_ms_ = ms;
}


}
