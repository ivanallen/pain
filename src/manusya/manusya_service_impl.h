#pragma once

#include "core/manusya.pb.h"

namespace pain::manusya {
class ManusyaServiceImpl : public ManusyaService {
public:
  ManusyaServiceImpl() {}
  virtual ~ManusyaServiceImpl() {}
  void create_chunk(google::protobuf::RpcController *cntl_base,
                    const CreateChunkRequest *request,
                    CreateChunkResponse *response,
                    google::protobuf::Closure *done) override;
};

} // namespace pain::manusya