#ifndef CR_DEVICE_H_
#define CR_DEVICE_H_
#include <memory>
#include <string>

#include "framework/device_base.h"
#include "protobuf/device_attributes.pb.h"
#include "core/base/macros.h"

namespace mr {

class Device : public DeviceBase {
 public:
  Device(Env* env, const DeviceAttributes& device_attributes);
  ~Device() override;

  const std::string& name() const { return device_attributes_.name(); }

  const std::string& device_type() const {
    return device_attributes_.device_type();
  }

  const DeviceAttributes& attributes() const override {
    return device_attributes_;
  }

  virtual void Compute() {
    LOG(INFO) << "TODO(wqx): Compute";
  }

 private:
  const DeviceAttributes device_attributes_;
  DISALLOW_COPY_AND_ASSIGN(Device);
};

} // namespace mr
#endif // CR_DEVICE_H_
