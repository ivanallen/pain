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
        return Status(ENOENT, fmt::format("NotFound:{}", status.ToString()));
    case rocksdb::Status::Code::kCorruption:
        return Status(EBADMSG, fmt::format("Corruption:{}", status.ToString()));
    case rocksdb::Status::Code::kNotSupported:
        return Status(ENOTSUP, fmt::format("NotSupported:{}", status.ToString()));
    case rocksdb::Status::Code::kInvalidArgument:
        return Status(EINVAL, fmt::format("InvalidArgument:{}", status.ToString()));
    case rocksdb::Status::Code::kIOError:
        return Status(EIO, fmt::format("IOError:{}", status.ToString()));
    case rocksdb::Status::Code::kMergeInProgress:
        return Status(EBUSY, fmt::format("MergeInProgress:{}", status.ToString()));
    case rocksdb::Status::Code::kIncomplete:
        return Status(EIO, fmt::format("Incomplete:{}", status.ToString()));
    case rocksdb::Status::Code::kShutdownInProgress:
        return Status(EIO, fmt::format("ShutdownInProgress:{}", status.ToString()));
    case rocksdb::Status::Code::kTimedOut:
        return Status(ETIMEDOUT, fmt::format("TimedOut:{}", status.ToString()));
    case rocksdb::Status::Code::kAborted:
        return Status(EIO, fmt::format("Aborted:{}", status.ToString()));
    case rocksdb::Status::Code::kBusy:
        return Status(EBUSY, fmt::format("Busy:{}", status.ToString()));
    case rocksdb::Status::Code::kExpired:
        return Status(EKEYEXPIRED, fmt::format("Expired:{}", status.ToString()));
    case rocksdb::Status::Code::kTryAgain:
        return Status(EAGAIN, fmt::format("TryAgain:{}", status.ToString()));
    case rocksdb::Status::Code::kCompactionTooLarge:
        return Status(EIO, fmt::format("CompactionTooLarge:{}", status.ToString()));
    case rocksdb::Status::Code::kColumnFamilyDropped:
        return Status(EIO, fmt::format("ColumnFamilyDropped:{}", status.ToString()));
    default:
        return Status(EIO, fmt::format("Unknown:{}", status.ToString()));
    }
}

} // namespace pain::common
