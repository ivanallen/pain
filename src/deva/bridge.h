#pragma once

#include <braft/raft.h>
#include <pain/base/future.h>
#include <pain/base/types.h>
#include <functional>
#include "deva/container_op.h"
#include "deva/op.h"
#include "deva/rsm.h"

namespace pain::deva {

template <typename ContainerType, OpType OpType, typename Request, typename Response>
void bridge(RsmPtr rsm, const Request& request, Response* response, std::move_only_function<void(Status)> cb) {
    // TODO: get rsm by pool id and partition id
    (new ContainerOp<ContainerType, Request, Response>(OpType,
                                                       rsm,
                                                       request,
                                                       response,
                                                       [cb = std::move(cb)](Status status) mutable {
                                                           cb(std::move(status));
                                                       }))
        ->apply();
}

// Future style
template <typename ContainerType, OpType OpType, typename Request, typename Response>
Future<Status> bridge(RsmPtr rsm, const Request& request, Response* response) {
    Promise<Status> promise;
    auto future = promise.get_future();
    bridge<ContainerType, OpType>(rsm, request, response, [promise = std::move(promise)](Status status) mutable {
        promise.set_value(std::move(status));
    });
    return future;
}

} // namespace pain::deva
