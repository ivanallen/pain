#pragma once

#include <pain/base/types.h>
#include "common/store.h"

namespace pain::common {

class TxnStore : public Store {
public:
    ~TxnStore() override = default;
    virtual Status commit() = 0;
    virtual Status rollback() = 0;

    Status close() override {
        return Status(ENOTSUP, "TxnStore does not support close");
    }
    Status recover(const char* from) override {
        std::ignore = from;
        return Status(ENOTSUP, "TxnStore does not support recover");
    }
    Status check_point(const char* to, std::vector<std::string>* files) override {
        std::ignore = to;
        std::ignore = files;
        return Status(ENOTSUP, "TxnStore does not support check_point");
    }
    std::shared_ptr<TxnStore> begin_txn() override {
        return nullptr;
    }
};

} // namespace pain::common
