#ifndef DR_MASTER_ENV_H_
#define DR_MASTER_ENV_H_
#include <functional>
#include <vector>

#include "public/session_options.h"

namespace mr {

class Device;
class Env;
class MasterSessionInterface;
class WorkerCacheInterface;

struct MasterEnv {
 Env* env = nullptr;
 WorkerCacheInterface* worker_cache = nullptr;
 std::vector<Device*> local_devices;

 std::function<MasterSessionInterface*(const SessionOptions&,
		                       MasterEnv*,
				       std::vector<Device*>*)>
	 master_session_factory;
};

} // namespace mr
#endif // DR_MASTER_ENV_H_
