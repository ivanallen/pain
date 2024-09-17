#include <brpc/channel.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <cstdlib>
#include <gflags/gflags.h>

#include "base/plog.h"
#include "base/spdlog_sink.h"
#include "base/tracer.h"
#include "spdlog/common.h"
#include <argparse/argparse.hpp>
#include <fmt/format.h>

namespace pain::sad {
extern argparse::ArgumentParser program;
void init(int argc, char *argv[]);
butil::Status execute(argparse::ArgumentParser &parser);
} // namespace pain::sad

int main(int argc, char *argv[]) {
  pain::sad::init(argc, argv);
  pain::base::LoggerOptions logger_options = {
      .file_name = "sad.log",
      .name = "sad",
      .level_log = spdlog::level::from_str(
          pain::sad::program.get<std::string>("--log-level")),
      .async_threads = 1,
  };
  static auto flush_log = pain::base::make_logger(logger_options);

  static pain::base::SpdlogSink spdlog_sink;
  logging::SetLogSink(&spdlog_sink);
  PLOG_WARN(("desc", "sad start"));
  pain::base::init_tracer("sad");

  auto status = pain::sad::execute(pain::sad::program);
  if (!status.ok()) {
    PLOG_ERROR(("desc", "sad exit")("status", status.error_str()));
    fmt::print(stderr, R"({{"status":{},"message":"{}"}}{})",
               status.error_code(), status.error_str(), "\n");
    pain::base::cleanup_tracer();
    return EXIT_FAILURE;
  }
  pain::base::cleanup_tracer();
  return 0;
}
