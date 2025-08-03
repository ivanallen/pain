#pragma once
#include <list>

#include "pain/chunk.h"
#include "pain/proto/pain.pb.h"
#include "base/uuid.h"

namespace pain {

class Controller;
class AppendRequest;
class AppendResponse;
class ReadRequest;
class ReadResponse;
class FileStreamImpl : public pain::proto::FileService {
public:
    void append(::google::protobuf::RpcController* controller,
                const ::pain::proto::AppendRequest* request,
                ::pain::proto::AppendResponse* response,
                ::google::protobuf::Closure* done) override;
    void read(::google::protobuf::RpcController* controller,
              const ::pain::proto::ReadRequest* request,
              ::pain::proto::ReadResponse* response,
              ::google::protobuf::Closure* done) override;
    void seal(::google::protobuf::RpcController* controller,
              const ::pain::proto::SealRequest* request,
              ::pain::proto::SealResponse* response,
              ::google::protobuf::Closure* done) override;
    void close(::google::protobuf::RpcController* controller,
               const ::pain::proto::CloseRequest* request,
               ::pain::proto::CloseResponse* response,
               ::google::protobuf::Closure* done) override;

private:
    ~FileStreamImpl() = default;
    UUID _uuid;
    std::list<Chunk> _chunks;
    friend class FileStream;
};

} // namespace pain
