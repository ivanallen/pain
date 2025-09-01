#pragma once
#include <pain/base/types.h>
#include <atomic>
#include <cstdint>
#include <fmt/format.h>
#include <boost/intrusive_ptr.hpp>
#include <magic_enum/magic_enum.hpp>

#define DEFINE_DEVA_OP(code, name, need_apply) k##name = ((code << 1) | (need_apply ? 1 : 0))

namespace pain::deva {

enum class OpType : uint32_t {
    kInvalid = 0,
    // DevaOp: 1 ~ 100
    DEFINE_DEVA_OP(1, CreateFile, true),
    DEFINE_DEVA_OP(2, CreateDir, true),
    DEFINE_DEVA_OP(3, RemoveFile, true),
    DEFINE_DEVA_OP(4, SealFile, true),
    DEFINE_DEVA_OP(5, CreateChunk, true),
    DEFINE_DEVA_OP(6, CheckInChunk, true),
    DEFINE_DEVA_OP(7, SealChunk, true),
    DEFINE_DEVA_OP(8, SealAndNewChunk, true),
    DEFINE_DEVA_OP(9, ReadDir, false),
    DEFINE_DEVA_OP(100, MaxDevaOp, true),
};

inline constexpr bool need_apply(OpType type) {
    return (static_cast<int>(type) & 1) == 1;
}

struct OpMeta {
    int32_t version; // op version
    OpType type;
    uint64_t timestamp;
    uint32_t size;
    char reserved[40]; // NOLINT(readability-magic-numbers)
};

static_assert(sizeof(OpMeta) == 64, "OpMeta size must be 64byte"); // NOLINT(readability-magic-numbers)

class Op;
using OpPtr = boost::intrusive_ptr<Op>;
class Op {
public:
    Op() = default;
    virtual ~Op() = default;
    virtual OpType type() const = 0;
    virtual void apply() = 0;
    virtual void on_apply(int64_t index) = 0;
    virtual void on_finish(Status status) = 0;
    virtual void encode(IOBuf* buf) = 0;
    virtual void decode(IOBuf* buf) = 0;

private:
    std::atomic<int> _use_count = 0;
    friend void intrusive_ptr_add_ref(Op* op) {
        ++op->_use_count;
    }
    friend void intrusive_ptr_release(Op* op) {
        if (op->_use_count.fetch_sub(1) == 1) {
            delete op;
        }
    }
};

class Rsm;
using RsmPtr = boost::intrusive_ptr<Rsm>;

void encode(OpPtr op, IOBuf* buf);
OpPtr decode(IOBuf* buf, std::move_only_function<OpPtr(OpType, IOBuf*)> decode);

} // namespace pain::deva

template <>
struct fmt::formatter<pain::deva::OpType> : public fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(pain::deva::OpType type, FormatContext& ctx) const {
        std::string_view name = magic_enum::enum_name(type);
        return fmt::formatter<std::string_view>::format(name, ctx);
    }
};

#undef DEFINE_DEVA_OP
