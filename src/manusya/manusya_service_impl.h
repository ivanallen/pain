#pragma once

#include "core/manusya.pb.h"

namespace pain::manusya {
class ManusyaServiceImpl : public ManusyaService {
public:
  ManusyaServiceImpl() {}
  virtual ~ManusyaServiceImpl() {}
  void create_chunk(google::protobuf::RpcController *controller,
                    const CreateChunkRequest *request,
                    CreateChunkResponse *response,
                    google::protobuf::Closure *done) override;
  void append_chunk(google::protobuf::RpcController *controller,
                    const AppendChunkRequest *request,
                    AppendChunkResponse *response,
                    google::protobuf::Closure *done) override;
};

} // namespace pain::manusya

#define DEFINE_SPAN(span, controller)                                          \
  brpc::Controller *cntl = static_cast<brpc::Controller *>(controller);        \
  opentelemetry::trace::StartSpanOptions span##_options;                       \
  span##_options.kind = opentelemetry::trace::SpanKind::kServer;               \
  auto span##_new_context = base::extract_context(cntl);                       \
  span##_options.parent =                                                      \
      opentelemetry::trace::GetSpan(span##_new_context)->GetContext();         \
  auto span = base::get_tracer("manusya")->StartSpan(                          \
      __func__,                                                                \
      {{opentelemetry::trace::SemanticConventions::kRpcSystem, "brpc"},        \
       {opentelemetry::trace::SemanticConventions::kRpcService,                \
        cntl->method()->service()->name()},                                    \
       {opentelemetry::trace::SemanticConventions::kRpcMethod,                 \
        cntl->method()->name()},                                               \
       {opentelemetry::trace::SemanticConventions::kClientAddress,             \
        butil::endpoint2str(cntl->remote_side()).c_str()},                     \
       {opentelemetry::trace::SemanticConventions::kServerAddress,             \
        butil::endpoint2str(cntl->local_side()).c_str()}},                     \
      span##_options);                                                         \
  auto span##_scope = base::Tracer::WithActiveSpan(span);