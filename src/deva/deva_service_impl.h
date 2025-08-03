#pragma once

#include "pain/proto/deva.pb.h"

namespace pain::deva {

class DevaServiceImpl : public pain::proto::deva::DevaService {
public:
    DevaServiceImpl();
    virtual ~DevaServiceImpl() {}
    void open(::google::protobuf::RpcController* controller,
              const pain::proto::deva::OpenRequest* request,
              pain::proto::deva::OpenResponse* response,
              ::google::protobuf::Closure* done) override;
    void close(::google::protobuf::RpcController* controller,
               const pain::proto::deva::CloseRequest* request,
               pain::proto::deva::CloseResponse* response,
               ::google::protobuf::Closure* done) override;
    void remove(::google::protobuf::RpcController* controller,
                const pain::proto::deva::RemoveRequest* request,
                pain::proto::deva::RemoveResponse* response,
                ::google::protobuf::Closure* done) override;
    void seal(::google::protobuf::RpcController* controller,
              const pain::proto::deva::SealRequest* request,
              pain::proto::deva::SealResponse* response,
              ::google::protobuf::Closure* done) override;
    void create_chunk(::google::protobuf::RpcController* controller,
                      const pain::proto::deva::CreateChunkRequest* request,
                      pain::proto::deva::CreateChunkResponse* response,
                      ::google::protobuf::Closure* done) override;
    void remove_chunk(::google::protobuf::RpcController* controller,
                      const pain::proto::deva::RemoveChunkRequest* request,
                      pain::proto::deva::RemoveChunkResponse* response,
                      ::google::protobuf::Closure* done) override;
    void seal_chunk(::google::protobuf::RpcController* controller,
                    const pain::proto::deva::SealChunkRequest* request,
                    pain::proto::deva::SealChunkResponse* response,
                    ::google::protobuf::Closure* done) override;
    void seal_and_new_chunk(::google::protobuf::RpcController* controller,
                            const pain::proto::deva::SealAndNewChunkRequest* request,
                            pain::proto::deva::SealAndNewChunkResponse* response,
                            ::google::protobuf::Closure* done) override;
};

} // namespace pain::deva
