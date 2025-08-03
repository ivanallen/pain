#include "deva/deva.h"
#include "base/plog.h"
#include "base/uuid.h"
#include "deva/macro.h"

namespace pain::deva {

Status Deva::Open(const pain::proto::deva::store::OpenRequest* request,
                  pain::proto::deva::store::OpenResponse* response,
                  int64_t index) {
    SPAN(span);
    auto uuid = UUID(request->uuid().high(), request->uuid().low());
    PLOG_INFO(("desc", "open")("index", index)("uuid", uuid.str()));

    auto meta = response->mutable_file_meta();
    meta->mutable_inode()->CopyFrom(request->uuid());
    return Status::OK();
}

Status Deva::Close(const pain::proto::deva::store::CloseRequest* request,
                   pain::proto::deva::store::CloseResponse* response,
                   int64_t index) {
    PLOG_INFO(("desc", "close")("index", index));
    return Status::OK();
}

Status Deva::Remove(const pain::proto::deva::store::RemoveRequest* request,
                    pain::proto::deva::store::RemoveResponse* response,
                    int64_t index) {
    PLOG_INFO(("desc", "remove")("index", index));
    return Status::OK();
}

Status Deva::Seal(const pain::proto::deva::store::SealRequest* request,
                  pain::proto::deva::store::SealResponse* response,
                  int64_t index) {
    PLOG_INFO(("desc", "seal")("index", index));
    return Status::OK();
}

Status Deva::CreateChunk(const pain::proto::deva::store::CreateChunkRequest* request,
                         pain::proto::deva::store::CreateChunkResponse* response,
                         int64_t index) {
    PLOG_INFO(("desc", "create_chunk")("index", index));
    return Status::OK();
}

Status Deva::RemoveChunk(const pain::proto::deva::store::RemoveChunkRequest* request,
                         pain::proto::deva::store::RemoveChunkResponse* response,
                         int64_t index) {
    PLOG_INFO(("desc", "remove_chunk")("index", index));
    return Status::OK();
}

Status Deva::SealChunk(const pain::proto::deva::store::SealChunkRequest* request,
                       pain::proto::deva::store::SealChunkResponse* response,
                       int64_t index) {
    PLOG_INFO(("desc", "seal_chunk")("index", index));
    return Status::OK();
}

Status Deva::SealAndNewChunk(const pain::proto::deva::store::SealAndNewChunkRequest* request,
                             pain::proto::deva::store::SealAndNewChunkResponse* response,
                             int64_t index) {
    PLOG_INFO(("desc", "seal_and_new_chunk")("index", index));
    return Status::OK();
}

Status Deva::save_snapshot(std::string_view path, std::vector<std::string>* files) {
    return Status::OK();
}

Status Deva::load_snapshot(std::string_view path) {
    return Status::OK();
}

} // namespace pain::deva
