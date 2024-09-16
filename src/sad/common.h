#pragma once

#include "brpc/controller.h"
#include <json2pb/pb_to_json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

namespace pain::sad {
json pb_to_json(const google::protobuf::Message &message);
void print(const brpc::Controller &cntl,
           const google::protobuf::Message &message);
} // namespace pain::sad