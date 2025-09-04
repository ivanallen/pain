#pragma once

#include <pain/base/types.h>
#include <rocksdb/status.h>
#include <fmt/format.h>

namespace pain::common {

inline Status convert_to_pain_status(const rocksdb::Status& status) {
    switch (status.code()) {
    case rocksdb::Status::Code::kOk:
        return Status::OK();
    case rocksdb::Status::Code::kNotFound:
        return Status(ENOENT, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kCorruption:
        return Status(EBADMSG, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kNotSupported:
        return Status(ENOTSUP, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kInvalidArgument:
        return Status(EINVAL, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kIOError:
        return Status(EIO, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kMergeInProgress:
        return Status(EBUSY, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kIncomplete:
        return Status(EIO, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kShutdownInProgress:
        return Status(EIO, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kTimedOut:
        return Status(ETIMEDOUT, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kAborted:
        return Status(EIO, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kBusy:
        return Status(EBUSY, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kExpired:
        return Status(EKEYEXPIRED, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kTryAgain:
        return Status(EAGAIN, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kCompactionTooLarge:
        return Status(EIO, fmt::format("{}", status.ToString()));
    case rocksdb::Status::Code::kColumnFamilyDropped:
        return Status(EIO, fmt::format("{}", status.ToString()));
    default:
        return Status(EIO, fmt::format("{}", status.ToString()));
    }
}

} // namespace pain::common
