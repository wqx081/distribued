#include "dr/server_interface.h"
#include "protobuf/mr_server.pb.h"

#include "core/base/logging.h"
#include "core/strings/str_util.h"
#include "core/strings/strcat.h"
#include "core/strings/numbers.h"

#include <gtest/gtest.h>

namespace mr {

Status ParseFlagsForTask(int argc, char* argv[], ServerDef* options) {
  options->set_protocol("grpc");
  if (argc == 1) {
    return Status::OK;	
  }

  std::string cluster_spec;
  int task_index = 0;
  int i = 1;
  while (i < argc) {
    std::vector<std::string> kv = str_util::Split(argv[i], ',');
    if (kv[0] == "--cluster_spec") {
      cluster_spec = kv[1];
    } else if (kv[0] == "job_name") {
      *options->mutable_job_name() = kv[1]; 
    } else if (kv[0] == "task_id") {
       strings::safe_strto32(kv[1], &task_index);
    } else {
      return Status(error::INVALID_ARGUMENT,
		      "Commandline option error: " + kv[0]);
    }
    ++i;
  }
  options->set_task_index(task_index);

  size_t my_num_tasks = 0;

  ClusterDef* const cluster = options->mutable_cluster();

  for (std::string& job_str : str_util::Split(cluster_spec, ',')) {
    JobDef* const job_def = cluster->add_job();
    const std::vector<std::string> job_pieces = 
	    str_util::Split(job_str, '|');
    DCHECK_EQ(2, job_pieces.size()) << job_str;
    const std::string& job_name = job_pieces[0];
    job_def->set_name(job_name);

    const StringPiece spec = job_pieces[1];
    const std::vector<std::string> host_ports = 
	    str_util::Split(spec, ';');
    for (size_t i = 0; i < host_ports.size(); ++i) {
      (*job_def->mutable_tasks())[i] = host_ports[i];
    }
    size_t num_tasks = host_ports.size();
    if (job_name == options->job_name()) {
      my_num_tasks = host_ports.size();
    }
    LOG(INFO) << "Peer " << job_name << " " << num_tasks << " {"
	      << str_util::Join(host_ports, ", ") << "}";
  }
  if (my_num_tasks == 0) {
    return Status(error::INVALID_ARGUMENT,
		    "Job name \"" + options->job_name() + "\""
		    + " does not appear in the cluster spec");
  }
  if ((size_t)options->task_index() >= my_num_tasks) {
    return Status(error::INVALID_ARGUMENT,
		   "task index " + std::to_string(options->task_index())
		   + " is invalid (job \"" +
		   options->job_name() +
		   "\" contains " + std::to_string(my_num_tasks) + " tasks");
  }
  return Status::OK;	
}

} // namespace mr

int g_argc;
char** g_argv;

TEST(MrServer, Basic) {
  mr::ServerDef server_def;
  std::unique_ptr<mr::ServerInterface> server;
  mr::Status s = mr::ParseFlagsForTask(g_argc, g_argv, &server_def);
  if (!s.ok()) {
    LOG(INFO) << s.ToString();
  }
  EXPECT_TRUE(s.ok());

  s = mr::NewServer(server_def, &server);
  if (!s.ok()) {
    LOG(INFO) << s.ToString();
  }
  EXPECT_TRUE(s.ok());
  server->Start();
  server->Join();
  EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
  g_argc = argc;
  g_argv = argv;
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
