#include "manusya/manusya_service_impl.h"
#include "base/brpc_text_map_carrier.h"
#include "base/plog.h"
#include "base/tracer.h"
#include "butil/endpoint.h"
#include <brpc/controller.h>

DEFINE_string(manusya_data_path, "./data",
              "The path to store the data of manusya");

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

  ChunkPtr chunk = Chunk::create();

  // BUGS: not thread safe
  _chunks[chunk->uuid()] = chunk;
  union {
    char uuid[16];
    struct {
      uint64_t low;
      uint64_t high;
    };
  } uuid;
  chunk->uuid().bytes(uuid.uuid);

  PLOG_INFO(("desc", "Created chunk")("uuid", chunk->uuid().str()));
  response->mutable_uuid()->set_low(uuid.low);
  response->mutable_uuid()->set_high(uuid.high);
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

void ManusyaServiceImpl::list_chunk(google::protobuf::RpcController *controller,
                                    const ListChunkRequest *request,
                                    ListChunkResponse *response,
                                    google::protobuf::Closure *done) {
  DEFINE_SPAN(span, controller);
  brpc::ClosureGuard done_guard(done);

  PLOG_INFO(("desc", "Received request")                                      //
            ("log_id", cntl->log_id())                                        //
            ("remote_side", butil::endpoint2str(cntl->remote_side()).c_str()) //
            ("attached", cntl->request_attachment().size()));

  for (const auto &[u, chunk] : _chunks) {
    union {
      char uuid[16];
      struct {
        uint64_t low;
        uint64_t high;
      };
    } uuid;
    u.bytes(uuid.uuid);
    auto c = response->add_uuids();
    c->set_low(uuid.low);
    c->set_high(uuid.high);
  }
}

} // namespace pain::manusya