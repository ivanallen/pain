#include "deva/container_op.h"
#include <pain/base/types.h>
#include <boost/assert.hpp>
#include "pain/proto/deva_store.pb.h"
#include "deva/deva.h"
#include "deva/op.h"

namespace pain::deva {

template <OpType OpType, typename Request, typename Response>
OpPtr create(int32_t version, RsmPtr rsm) {
    Request request;
    OpPtr op = nullptr;

    if constexpr (OpType < OpType::kMaxDevaOp) {
        op = new ContainerOp<Deva, Request, Response>(version, OpType, rsm, request, nullptr);
    }
    return op;
}

#define BRANCH(name)                                                                                                   \
    case OpType::k##name:                                                                                              \
        return create<OpType::k##name, proto::deva::store::name##Request, proto::deva::store::name##Response>(version, \
                                                                                                              rsm);    \
        break;

OpPtr decode(int32_t version, OpType op_type, IOBuf* buf, RsmPtr rsm) {
    auto op = [](int32_t version, OpType op_type, RsmPtr rsm) -> OpPtr {
        switch (op_type) {
            BRANCH(CreateFile)
            BRANCH(CreateDir)
            BRANCH(ReadDir)
            BRANCH(RemoveFile)
            BRANCH(SealFile)
            BRANCH(CreateChunk)
            BRANCH(CheckInChunk)
            BRANCH(SealChunk)
            BRANCH(SealAndNewChunk)
        default:
            BOOST_ASSERT_MSG(false, fmt::format("unknown op type: {}", op_type).c_str());
        }
        return nullptr;
    }(version, op_type, rsm);
    op->decode(buf);
    return op;
}

#undef BRANCH

} // namespace pain::deva
