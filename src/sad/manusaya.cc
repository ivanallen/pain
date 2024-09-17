#include <argparse/argparse.hpp>
#include <brpc/channel.h>
#include <brpc/controller.h>
#include <butil/guid.h>
#include <butil/status.h>
#include <core/manusya.pb.h>
#include <fmt/format.h>
#include <json2pb/pb_to_json.h>

#include "base/tracer.h"
#include "sad/common.h"
#include "sad/macro.h"

using Status = butil::Status;

#define MANUSYA_ARGS(cmd) ARGS(cmd, manusya_parser)

namespace pain::sad {
extern argparse::ArgumentParser program;
}
namespace pain::sad::manusya {
argparse::ArgumentParser manusya_parser("manusya", "1.0",
                                        argparse::default_arguments::none);

RUN(program.add_subparser(manusya_parser));
RUN(manusya_parser.add_description("send cmd to manusya server")
        .add_argument("--host")
        .default_value(std::string("127.0.0.1:8003")));

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
  SPAN(span);
  for (const auto &[name, func] : subcommands) {
    if (parser.is_subcommand_used(name)) {
      return func(parser.at<argparse::ArgumentParser>(name));
    }
  }
  std::cerr << parser;
  std::exit(1);
}

RUN(MANUSYA_ARGS(create_chunk).add_description("create chunk"));
COMMAND(create_chunk) {
  SPAN(span);
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
  base::inject_tracer(&cntl);
  stub.create_chunk(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    return Status(cntl.ErrorCode(), cntl.ErrorText());
  }

  print(cntl, &response);
  return Status::OK();
}

RUN(MANUSYA_ARGS(append_chunk)
        .add_description("append chunk")
        .add_argument("-c", "--chunk-id")
        .required()
        .help("chunk uuid, such as 123e4567-e89b-12d3-a456-426655440000"));
COMMAND(append_chunk) {
  SPAN(span);
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
  base::inject_tracer(&cntl);

  request.mutable_uuid()->set_low(100);
  request.mutable_uuid()->set_high(200);
  cntl.request_attachment().append("test data");
  stub.append_chunk(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    return Status(cntl.ErrorCode(), cntl.ErrorText());
  }

  print(cntl, &response);
  return Status::OK();
}

} // namespace pain::sad::manusya