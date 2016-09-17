#if 0
FilePath: distributed_runtime/rpc/grpc_tensorflow_server.cc

int main(int argc, char** argv) {

  //初始化命令行参数  
  InitMain(argv[0], &argc, &argv);
  
  //服务的定义
  ServerDef server_def;

  // 解析命令行参数
  Status s = ParseFlagsForTask(argc, argv, &server_def);

  if (!s.ok()) return;

  // 定义服务
  std::unique_ptr<ServerInterface> server;
  // 生成服务
  NewServer(server_def, &server);
  // 运行服务
  server->Start();
  server->Join();
  return 0;
}

命令行启动方式如下:
./mr_server --cluster_spec=SPEC --job_name=NAME -task_id=ID

SPEC => <JOB>(,<JOB>)*
JOB  => <NAME>|<HOST:PORT>(;<HOST:PORT>)*

#endif
