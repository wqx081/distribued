#include "dr/rpc/grpc_server.h"
#include "protobuf/mr_server.pb.h"

#include "core/base/logging.h"

namespace mr {

GrpcServer::GrpcServer(const ServerDef& server_def)
  : server_def_(server_def) {}

GrpcServer::~GrpcServer() {}

Status GrpcServer::Start() {
  LOG(INFO) << "GrpcServer::Start()";
  return Status::OK;
}

Status GrpcServer::Stop() {
  LOG(INFO) << "GrpcServer::Stop()";
  return Status::OK;
}

Status GrpcServer::Join() {
  LOG(INFO) << "GrpcServer::Join()";
  return Status::OK;
}

const std::string GrpcServer::target() const {
  return "grpc://localhost";
}

// static
Status GrpcServer::Create(const ServerDef& server_def,
    std::unique_ptr<ServerInterface>* out_server) {
  std::unique_ptr<GrpcServer> ret(new GrpcServer(server_def));
  *out_server = std::move(ret);
  return Status::OK;
}

// For Abstract Factory
namespace {

class GrpcServerFactory : public ServerFactory {
 public:
  bool AcceptsOptions(const ServerDef& server_def) override {
    return server_def.protocol() == "grpc";
  }
  Status NewServer(const ServerDef& server_def,
      std::unique_ptr<ServerInterface>* out_server) override {
    return GrpcServer::Create(server_def, out_server);  
  }
};

// Registers
class GrpcServerRegistrar {
 public:
  GrpcServerRegistrar() {
    ServerFactory::Register("GRPC_SERVER", new GrpcServerFactory());
  }
};
static GrpcServerRegistrar registrar;

} // namespace

} // namespace mr
