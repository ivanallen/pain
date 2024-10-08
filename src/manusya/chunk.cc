#include "manusya/chunk.h"
#include <fcntl.h>
#include <cerrno>
#include <format>
#include <mutex>
#include "base/plog.h"
#include "manusya/file_handle.h"
#include "manusya/macro.h"

namespace pain::manusya {

Status Chunk::append(const IOBuf& buf, uint64_t offset) {
    SPAN(span);
    std::unique_lock lock(_mutex);
    if (offset < _size) {
        return Status(EINVAL, std::format("invalid offset at {}@{}, current size:{}", offset, buf.size(), _size));
    }

    if (offset != _size) {
        if (!_options.append_out_of_order) {
            return Status(EINVAL,
                          std::format("append out of order at {}@{}, current size:{}", offset, buf.size(), _size));
        }
        AppendRequestPtr rq(new AppendRequest());
        rq->offset = offset;
        rq->buf = buf;
        rq->chunk = this;
        rq->start = butil::cpuwide_time_ns();
        rq->span = span;
        auto abstime = butil::microseconds_from_now(5 * 1000 * 1000);

        intrusive_ptr_add_ref(rq.get());
        bthread_timer_add(
            &rq->timer,
            abstime,
            [](void* arg) {
                auto rq = static_cast<AppendRequest*>(arg);
                auto root_scope = Tracer::WithActiveSpan(rq->span);
                SPAN(span, "timeout");
                std::unique_lock lock(rq->chunk->_mutex);
                if (rq->promise.is_ready()) {
                    PLOG_DEBUG(("desc", "append request is ready"));
                    return;
                }
                rq->unlink();
                rq->promise.set_value(Status(ETIMEDOUT, "append request timeout"));
                intrusive_ptr_release(rq);
            },
            rq.get());

        _append_request_queue.insert(*rq);
        lock.unlock();
        auto status = rq->promise.get_future().get();
        rq->end = butil::cpuwide_time_ns();

        return status;
    }
    if (_state == ChunkState::SEALED) {
        return Status(EINVAL, "chunk is sealed");
    }
    if (_state == ChunkState::INIT) {
        _state = ChunkState::OPEN;
    }
    auto status = _fh->append(offset, buf).get();

    if (!status.ok()) {
        return status;
    }

    _size += buf.size();

    while (!_append_request_queue.empty()) {
        auto it = _append_request_queue.begin();
        auto rq = &*it;
        if (rq->offset < _size) {
            rq->unlink();
            if (!rq->promise.is_ready()) {
                rq->promise.set_value(
                    Status(EINVAL,
                           std::format("invalid offset at {}@{}, current size:{}", rq->offset, rq->buf.size(), _size)));
            }
        } else if (rq->offset == _size) {
            auto status = _fh->append(rq->offset, rq->buf).get();
            if (status.ok()) {
                _size += rq->buf.size();
            }
            rq->unlink();
            if (!rq->promise.is_ready()) {
                rq->promise.set_value(status);
            }
        } else {
            break;
        }
    }
    return Status::OK();
}

Status Chunk::seal() {
    SPAN(span);
    std::lock_guard lock(_mutex);
    _state = ChunkState::SEALED;
    auto status = _fh->seal().get();
    return status;
}

Status Chunk::read(uint64_t offset, uint64_t size, IOBuf* buf) const {
    SPAN(span);
    std::lock_guard lock(_mutex);
    if (offset + size > _size) {
        return Status(EINVAL,
                      std::format("read out of range, offset:{}, size:{}, current size:{}", offset, size, _size));
    }
    FileHandlePtr fh;
    auto status = _store->open(_uuid.str().c_str(), O_RDONLY, &fh).get();
    if (!status.ok()) {
        return status;
    }
    status = fh->read(offset, size, buf).get();
    return status;
}

Status Chunk::create(const ChunkOptions& options, StorePtr store, ChunkPtr* chunk) {
    SPAN(span);
    static thread_local UUIDv4::UUIDGenerator<std::mt19937_64> uuid_gen;
    auto c = ChunkPtr(new Chunk());
    c->_uuid = uuid_gen.getUUID();
    c->_options = options;
    c->_store = store;
    c->_size = 0;
    auto status = store->open(c->_uuid.str().c_str(), O_CREAT | O_WRONLY | O_EXCL, &c->_fh).get();

    if (!status.ok()) {
        return status;
    }

    status = c->_fh->set_attr("user.append-out-of-order", options.append_out_of_order ? "1" : "0").get();
    if (!status.ok()) {
        return status;
    }
    status = c->_fh->set_attr("user.digest", options.digest ? "1" : "0").get();
    if (!status.ok()) {
        return status;
    }

    *chunk = c;
    return Status::OK();
}

Status Chunk::create(const ChunkOptions& options, StorePtr store, const UUID& uuid, ChunkPtr* chunk) {
    SPAN(span);
    auto c = ChunkPtr(new Chunk());
    c->_uuid = uuid;
    c->_options = options;
    c->_store = store;
    auto status = store->open(c->_uuid.str().c_str(), O_RDONLY, &c->_fh).get();

    if (!status.ok()) {
        return status;
    }

    status = c->_fh->size(&c->_size).get();

    if (!status.ok()) {
        return status;
    }

    std::string value;
    status = c->_fh->get_attr("user.append-out-of-order", &value).get();
    if (status.ok()) {
        c->_options.append_out_of_order = value == "1";
    } else {
        PLOG_ERROR(("desc", "failed to get append-out-of-order attribute")("error", status.error_str()));
    }
    status = c->_fh->get_attr("user.digest", &value).get();
    if (status.ok()) {
        c->_options.digest = value == "1";
    } else {
        PLOG_ERROR(("desc", "failed to get digest attribute")("error", status.error_str()));
    }

    *chunk = c;
    return Status::OK();
}

} // namespace pain::manusya
