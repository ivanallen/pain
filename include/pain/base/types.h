#pragma once

#include <butil/iobuf.h>
#include <butil/status.h>
#include <fmt/format.h>

namespace pain {
using Status = butil::Status;
using IOBuf = butil::IOBuf;
} // namespace pain

template <>
struct fmt::formatter<pain::Status> : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(pain::Status status, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), R"({{"code":{},"message":"{}"}})", status.error_code(), status.error_str());
    }
};
