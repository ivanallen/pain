#include "pain/file_stream.h"
#include "pain/file_stream_impl.h"

namespace pain {

FileStream::~FileStream() {
    delete _impl;
}

void FileStream::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {
    _impl->CallMethod(method, controller, request, response, done);
}

} // namespace pain
