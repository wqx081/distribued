#include "dr/server_interface.h"

#include "core/base/logging.h"

#include <unordered_map>
#include <mutex>

namespace mr {

namespace {

std::mutex* get_server_factory_lock() {
  static std::mutex server_factory_lock;
  return &server_factory_lock;  
}

typedef std::unordered_map<std::string, ServerFactory*> ServerFactories;
ServerFactories* server_factories() {
  static ServerFactories* factories = new ServerFactories;
  return factories;
}

} // namespace

// static
void ServerFactory::Register(const std::string& server_type,
		             ServerFactory* factory) {
  std::lock_guard<std::mutex> l(*get_server_factory_lock());
  if (!server_factories()->insert({server_type, factory}).second) {
    LOG(ERROR) << "Two server factories are begin registered under "
	       << server_type;
  }
}

// static
Status ServerFactory::GetFactory(const ServerDef& server_def,
		                       ServerFactory** out_factory) {
  std::lock_guard<std::mutex> l(*get_server_factory_lock());
  for (const auto& server_factory : *server_factories()) {
    if (server_factory.second->AcceptsOptions(server_def)) {
      *out_factory = server_factory.second;
      return Status::OK;
    }
  }
  return Status(error::NOT_FOUND,
		"No server factory registered for the given ServerDef: " +
		server_def.DebugString());
}

Status NewServer(const ServerDef& server_def,
		 std::unique_ptr<ServerInterface>* out_server) {
  ServerFactory* factory;
  RETURN_IF_ERROR(ServerFactory::GetFactory(server_def, &factory));
  return factory->NewServer(server_def, out_server);
}

} // namespace mr
