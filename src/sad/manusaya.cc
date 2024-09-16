#include "sad/common.h"
#include <argparse/argparse.hpp>
#include <brpc/channel.h>
#include <brpc/controller.h>
#include <butil/guid.h>
#include <butil/status.h>
#include <core/manusya.pb.h>
#include <fmt/format.h>
#include <json2pb/pb_to_json.h>

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
  auto host = args.get<std::string>("--host");
  brpc::Channel channel;
  brpc::ChannelOptions options;
  if (channel.Init(host.c_str(), &options) != 0) {
    return Status(EAGAIN, "Fail to initialize channel");
  }

  brpc::Controller cntl;
  pain::manusya::CreateChunkRequest request;
  pain::manusya::CreateChunkResponse response;
  pain::manusya::ManusyaService_Stub stub(&channel);
  stub.create_chunk(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    return Status(cntl.ErrorCode(), cntl.ErrorText());
  }

  print(cntl, response);
  return Status::OK();
}

COMMAND(append_chunk) {
  auto chunk_id = args.get<std::string>("--chunk-id");
  auto host = args.get<std::string>("--host");

  brpc::Channel channel;
  brpc::ChannelOptions options;
  if (channel.Init(host.c_str(), &options) != 0) {
    return Status(EAGAIN, "Fail to initialize channel");
  }

  brpc::Controller cntl;
  pain::manusya::AppendChunkRequest request;
  pain::manusya::AppendChunkResponse response;
  pain::manusya::ManusyaService_Stub stub(&channel);

  request.mutable_uuid()->set_low(100);
  request.mutable_uuid()->set_high(200);
  cntl.request_attachment().append("test data");
  stub.append_chunk(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    return Status(cntl.ErrorCode(), cntl.ErrorText());
  }

  print(cntl, response);

  return Status::OK();
}

} // namespace pain::sad::manusya