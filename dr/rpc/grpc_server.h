#ifndef MR_DR_RPC_GRPC_SERVER_H_
#define MR_DR_RPC_GRPC_SERVER_H_
#include <memory>

#include "dr/server_interface.h"

namespace mr {

class GrpcServer : public ServerInterface {
 protected:
  //把构造函数限制为保护域，使得我们只能通过抽象工厂来创建对象
  GrpcServer(const ServerDef& server_def);

 public:
  static Status Create(const ServerDef& server_def,
		       std::unique_ptr<ServerInterface>* out_server);
  virtual ~GrpcServer();

  // From ServerInterface methods.
  Status Start() override;
  Status Stop() override;
  Status Join() override;
  const std::string target() const override;

 private:
  const ServerDef server_def_;
};

} // namespace mr
#endif // MR_DR_RPC_GRPC_SERVER_H_
