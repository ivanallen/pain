#include <spdlog/spdlog.h>

#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include "core/manusya.pb.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with response");
DEFINE_int32(port, 8003, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");

namespace pain::manusya {
class ManusyaServiceImpl : public ManusyaService {
public:
  ManusyaServiceImpl() {}
  virtual ~ManusyaServiceImpl() {}
  virtual void create_chunk(google::protobuf::RpcController *cntl_base,
                            const CreateChunkRequest *request,
                            CreateChunkResponse *response,
                            google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);

    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

    LOG(INFO) << "Received request[log_id=" << cntl->log_id() << "] from "
              << cntl->remote_side() << ": "
              << " (attached=" << cntl->request_attachment() << ")";

    response->mutable_uuid()->set_value("abcd");
  }
};

} // namespace pain::manusya

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  brpc::Server server;

  pain::manusya::ManusyaServiceImpl manusya_service_impl;

  if (server.AddService(&manusya_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Fail to add service";
    return -1;
  }

  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;
  if (server.Start(FLAGS_port, &options) != 0) {
    LOG(ERROR) << "Fail to start EchoServer";
    return -1;
  }

  server.RunUntilAskedToQuit();
  return 0;
}