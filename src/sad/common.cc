#include "sad/common.h"
#include "base/tracer.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;
using Status = butil::Status;

namespace pain::sad {

json pb_to_json(const google::protobuf::Message &message) {
  json2pb::Pb2JsonOptions options;
  options.enable_protobuf_map = true;
  options.enum_option = json2pb::OUTPUT_ENUM_BY_NAME;
  options.jsonify_empty_array = true;
  options.pretty_json = true;
  options.always_print_primitive_fields = true;
  std::string out;
  json2pb::ProtoMessageToJson(message, &out);
  auto j = json::parse(out);
  return j;
}

void print(const Status &status) {
  json out;
  out["header"] = {
      {"status", status.error_code()},
      {"message", status.error_str()},
      {"trace_id", base::get_current_trace_id()},
  };
  fmt::print("{}\n", out.dump(2));
}

void print(const brpc::Controller &cntl,
           const google::protobuf::Message *message) {
  json out;
  out["header"] = {
      {"status", cntl.ErrorCode()},
      {"message", cntl.ErrorText()},
      {"trace_id", base::get_current_trace_id()},
      {"remote_side", butil::endpoint2str(cntl.remote_side()).c_str()},
  };

  if (message != nullptr) {
    auto j = pb_to_json(*message);
    out.merge_patch(j);
  }
  fmt::print("{}\n", out.dump(2));
}

std::string format_command(const std::string &name) {
  std::string name_;
  for (auto c : name) {
    if (c == '_') {
      c = '-';
    }
    name_ += c;
  }
  return name_;
}

} // namespace pain::sad