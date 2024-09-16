#include <argparse/argparse.hpp>
#include <butil/status.h>
#include <cerrno>
#include <fmt/format.h>

using Status = butil::Status;

#define COMMAND(name)                                                          \
  butil::Status name(argparse::ArgumentParser &);                              \
  struct __add_##name {                                                        \
    __add_##name() { add(#name, name); }                                       \
  } __add_##name##_instance;                                                   \
  butil::Status name(argparse::ArgumentParser &args)

namespace pain::sad::manusya {

static std::map<std::string, std::function<Status(argparse::ArgumentParser &)>>
    subcommands = {};

void add(const std::string &name,
         std::function<Status(argparse::ArgumentParser &parser)> func) {
  std::string name_;
  for (auto c : name) {
    if (c == '_') {
      c = '-';
    }
    name_ += c;
  }
  subcommands[name_] = func;
}

Status execute(argparse::ArgumentParser &parser) {
  for (const auto &[name, func] : subcommands) {
    if (parser.is_subcommand_used(name)) {
      return func(parser.at<argparse::ArgumentParser>(name));
    }
  }
  std::cerr << parser;
  std::exit(1);
}

COMMAND(create_chunk) {
  fmt::println("create chunk");
  return Status::OK();
}

COMMAND(append_chunk) {
  fmt::println("append chunk");
  auto chunk_id = args.get<std::string>("--chunk-id");
  auto host = args.get<std::string>("--host");
  fmt::println("chunk id: {}", chunk_id);
  fmt::println("host: {}", host);
  return Status::OK();
}

} // namespace pain::sad::manusya