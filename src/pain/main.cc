#include <brpc/channel.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <gflags/gflags.h>

#include "base/plog.h"
#include "base/spdlog_sink.h"
#include "spdlog/common.h"
#include <argparse/argparse.hpp>

int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("program_name");
  program.add_argument("--log-level")
      .default_value(std::string("debug"))
      .required()
      .help("specify the log level");

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  pain::base::LoggerOptions logger_options = {
      .file_name = "pain.log",
      .name = "pain",
      .level_log =
          spdlog::level::from_str(program.get<std::string>("--log-level")),
      .async_threads = 1,
  };
  static auto flush_log = pain::base::make_logger(logger_options);

  static pain::base::SpdlogSink spdlog_sink;
  logging::SetLogSink(&spdlog_sink);
  PLOG_WARN(("desc", "pain start"));

  return 0;
}
