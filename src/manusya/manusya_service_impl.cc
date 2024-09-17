#include "manusya/manusya_service_impl.h"
#include "base/brpc_text_map_carrier.h"
#include "base/plog.h"
#include "base/tracer.h"
#include "butil/endpoint.h"
#include <brpc/controller.h>

namespace pain::manusya {

void ManusyaServiceImpl::create_chunk(
    google::protobuf::RpcController *controller,
    const CreateChunkRequest *request, CreateChunkResponse *response,
    google::protobuf::Closure *done) {
  DEFINE_SPAN(span, controller);
  brpc::ClosureGuard done_guard(done);

  PLOG_INFO(("desc", "Received request")                                      //
            ("log_id", cntl->log_id())                                        //
            ("remote_side", butil::endpoint2str(cntl->remote_side()).c_str()) //
            ("attached", cntl->request_attachment().size()));

  response->mutable_uuid()->set_low(100);
  response->mutable_uuid()->set_high(200);
}

void ManusyaServiceImpl::append_chunk(
    google::protobuf::RpcController *controller,
    const AppendChunkRequest *request, AppendChunkResponse *response,
    google::protobuf::Closure *done) {
  DEFINE_SPAN(span, controller);
  brpc::ClosureGuard done_guard(done);

  PLOG_INFO(("desc", "Received request")                                      //
            ("log_id", cntl->log_id())                                        //
            ("remote_side", butil::endpoint2str(cntl->remote_side()).c_str()) //
            ("attached", cntl->request_attachment().size()));

  std::cout << cntl->request_attachment() << std::endl;
}

} // namespace pain::manusya