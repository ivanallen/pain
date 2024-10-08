#include <brpc/server.h>
#include "base/plog.h"
#include "base/scope_exit.h"
#include "base/spdlog_sink.h"
#include "base/tracer.h"
#include "manusya/bank.h"
#include "manusya/manusya_service_impl.h"

DEFINE_string(manusya_listen_address, "127.0.0.1:8101", "Listen address of manusya");
DEFINE_int32(idle_timeout_s,
             -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_string(log_level, "debug", "Log level");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    pain::LoggerOptions logger_options = {
        .file_name = "manusya.log",
        .name = "manusya",
        .level_log = spdlog::level::from_str(FLAGS_log_level),
        .async_threads = 1,
    };
    static auto flush_log = pain::make_logger(logger_options);

    static pain::SpdlogSink spdlog_sink;
    logging::SetLogSink(&spdlog_sink);

    brpc::Server server;

    auto status = pain::manusya::Bank::instance().load();
    if (!status.ok()) {
        LOG(ERROR) << "Failed to load bank";
        return -1;
    }

    pain::manusya::ManusyaServiceImpl manusya_service_impl;
    pain::init_tracer("manusya");
    auto stop_tracer = pain::make_scope_exit([]() { pain::cleanup_tracer(); });

    if (server.AddService(&manusya_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_manusya_listen_address.c_str(), &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    server.RunUntilAskedToQuit();
    return 0;
}
