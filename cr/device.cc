#include "cr/device.h"

namespace mr {

Device::Device(Env* env, const DeviceAttributes& device_attributes)
  : DeviceBase(env), device_attributes_(device_attributes) {}

Device::~Device() {}

} // namespace mr
