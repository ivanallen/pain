#pragma once

#include <brpc/controller.h>
#include <json2pb/pb_to_json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

namespace pain::sad {
json pb_to_json(const google::protobuf::Message &message);
void print(const butil::Status &status);
void print(const brpc::Controller &cntl,
           const google::protobuf::Message *message = nullptr);
std::string format_command(const std::string &name);
} // namespace pain::sad