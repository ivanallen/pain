#include "argparse/argparse.hpp"
#include "base/tracer.h"
#include "sad/macro.h"
#include <butil/status.h>
#include <fmt/format.h>

using Status = butil::Status;

namespace pain::sad {
namespace manusya {

Status execute(argparse::ArgumentParser &parser);

} // namespace manusya

Status execute(argparse::ArgumentParser &parser) {
  SPAN(span);
  if (parser.is_subcommand_used("manusya")) {
    return manusya::execute(parser.at<argparse::ArgumentParser>("manusya"));
  }
  std::cerr << parser;
  std::exit(1);
}

} // namespace pain::sad