#pragma once

#include "pain/proto/manusya.pb.h"

namespace pain::manusya {
class ManusyaServiceImpl : public pain::proto::manusya::ManusyaService {
public:
    ManusyaServiceImpl();
    virtual ~ManusyaServiceImpl() {}
    void create_chunk(google::protobuf::RpcController* controller,
                      const pain::proto::manusya::CreateChunkRequest* request,
                      pain::proto::manusya::CreateChunkResponse* response,
                      google::protobuf::Closure* done) override;
    void append_chunk(google::protobuf::RpcController* controller,
                      const pain::proto::manusya::AppendChunkRequest* request,
                      pain::proto::manusya::AppendChunkResponse* response,
                      google::protobuf::Closure* done) override;
    void list_chunk(google::protobuf::RpcController* controller,
                    const pain::proto::manusya::ListChunkRequest* request,
                    pain::proto::manusya::ListChunkResponse* response,
                    google::protobuf::Closure* done) override;

    void read_chunk(google::protobuf::RpcController* controller,
                    const pain::proto::manusya::ReadChunkRequest* request,
                    pain::proto::manusya::ReadChunkResponse* response,
                    google::protobuf::Closure* done) override;
    void seal_chunk(google::protobuf::RpcController* controller,
                    const pain::proto::manusya::SealChunkRequest* request,
                    pain::proto::manusya::SealChunkResponse* response,
                    google::protobuf::Closure* done) override;
    void remove_chunk(google::protobuf::RpcController* controller,
                      const pain::proto::manusya::RemoveChunkRequest* request,
                      pain::proto::manusya::RemoveChunkResponse* response,
                      google::protobuf::Closure* done) override;
    void query_chunk(google::protobuf::RpcController* controller,
                     const pain::proto::manusya::QueryChunkRequest* request,
                     pain::proto::manusya::QueryChunkResponse* response,
                     google::protobuf::Closure* done) override;
};

} // namespace pain::manusya
