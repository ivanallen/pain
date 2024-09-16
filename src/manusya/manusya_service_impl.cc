#include "manusya/manusya_service_impl.h"
#include "base/plog.h"
#include "butil/endpoint.h"
#include <brpc/controller.h>

namespace pain::manusya {

void ManusyaServiceImpl::create_chunk(
    google::protobuf::RpcController *cntl_base,
    const CreateChunkRequest *request, CreateChunkResponse *response,
    google::protobuf::Closure *done) {
  brpc::ClosureGuard done_guard(done);

  brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

  PLOG_INFO(("desc", "Received request")                                      //
            ("log_id", cntl->log_id())                                        //
            ("remote_side", butil::endpoint2str(cntl->remote_side()).c_str()) //
            ("attached", cntl->request_attachment().size()));

  response->mutable_uuid()->set_low(100);
  response->mutable_uuid()->set_high(200);
}
} // namespace pain::manusya