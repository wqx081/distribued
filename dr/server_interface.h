//Design pattern: Abstract Factory
//
#ifndef DR_SERVER_INTERFACE_H_
#define DR_SERVER_INTERFACE_H_

#include "protobuf/mr_server.pb.h"

#include <memory>

#include "core/base/status.h"
#include "core/base/macros.h"

namespace mr {

class ServerInterface {
 public:
  ServerInterface() {}
  virtual ~ServerInterface() {}

  virtual Status Start() = 0;
  virtual Status Stop() = 0;
  virtual Status Join() = 0;
  virtual const std::string target() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServerInterface);
};

class ServerFactory {
 public:
  virtual Status NewServer(const ServerDef& server_def,
		           std::unique_ptr<ServerInterface>* out_server) = 0;
  virtual bool AcceptsOptions(const ServerDef& server_def) = 0;
  virtual ~ServerFactory() {}

  static void Register(const std::string& server_type, ServerFactory* factory);
  static Status GetFactory(const ServerDef& server_def,
		           ServerFactory** out_server);
};

Status NewServer(const ServerDef& server_def,
		 std::unique_ptr<ServerInterface>* out_server);

} // namespace mr
#endif // DR_SERVER_INTERFACE_H_
