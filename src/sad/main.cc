#include <brpc/channel.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <gflags/gflags.h>

#include "base/plog.h"
#include "base/spdlog_sink.h"
#include "core/manusya.pb.h"
#include "spdlog/common.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with requests");
DEFINE_string(protocol, "baidu_std",
              "Protocol type. Defined in src/brpc/options.proto");
DEFINE_string(connection_type, "",
              "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:8003", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_string(log_level, "debug", "log level");

void HandleCreateChunkResponse(brpc::Controller *cntl,
                               pain::manusya::CreateChunkResponse *response) {
  std::unique_ptr<brpc::Controller> cntl_guard(cntl);
  std::unique_ptr<pain::manusya::CreateChunkResponse> response_guard(response);

  if (cntl->Failed()) {
    LOG(WARNING) << "Fail to send CreateChunk, " << cntl->ErrorText();
    return;
  }
  LOG(INFO) << "Received response from " << cntl->remote_side() << ": "
            << response->uuid().value() << " latency=" << cntl->latency_us()
            << "us";
}

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  pain::base::LoggerOptions logger_options = {
      .file_name = "sad.log",
      .name = "sad",
      .level_log = spdlog::level::from_str(FLAGS_log_level),
      .async_threads = 1,
  };
  static auto flush_log = pain::base::make_logger(logger_options);

  static pain::base::SpdlogSink spdlog_sink;
  logging::SetLogSink(&spdlog_sink);
  PLOG_INFO(("desc", "sad start"));

  brpc::Channel channel;

  brpc::ChannelOptions options;
  options.protocol = FLAGS_protocol;
  options.connection_type = FLAGS_connection_type;
  options.timeout_ms = FLAGS_timeout_ms;
  options.max_retry = FLAGS_max_retry;
  if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(),
                   &options) != 0) {
    PLOG_ERROR(("desc", "Fail to initialize channel"));
    return -1;
  }

  pain::manusya::ManusyaService_Stub stub(&channel);

  int log_id = 0;
  while (!brpc::IsAskedToQuit()) {
    pain::manusya::CreateChunkResponse *response =
        new pain::manusya::CreateChunkResponse();
    brpc::Controller *cntl = new brpc::Controller();

    pain::manusya::CreateChunkRequest request;

    cntl->set_log_id(log_id++);

    google::protobuf::Closure *done =
        brpc::NewCallback(&HandleCreateChunkResponse, cntl, response);
    stub.create_chunk(cntl, &request, response, done);

    sleep(1);
  }

  LOG(INFO) << "sad is going to quit";
  return 0;
}
