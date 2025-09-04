#pragma once

#include <pain/base/types.h>
#include "common/txn_store.h"

namespace pain::common {

class TxnManager {
public:
    static const TxnManager& instance() {
        static TxnManager s_instance;
        return s_instance;
    }

    bool in_txn() const {
        return g_txn_store != nullptr;
    }

    TxnStore* get_txn_store() const {
        return g_txn_store;
    }

    void begin(TxnStore* txn_store) const {
        g_txn_store = txn_store;
    }

    Status commit() const {
        auto status = g_txn_store->commit();
        g_txn_store = nullptr;
        return status;
    }

    Status rollback() const {
        auto status = g_txn_store->rollback();
        g_txn_store = nullptr;
        return status;
    }

private:
    static thread_local TxnStore* g_txn_store;
};

class TxnGuard {
public:
    TxnGuard(TxnStore* txn_store) {
        TxnManager::instance().begin(txn_store);
    }

    ~TxnGuard() {
        if (_status.ok()) {
            TxnManager::instance().commit();
        } else {
            TxnManager::instance().rollback();
        }
    }

    void operator+(std::function<Status(TxnStore*)> cb) {
        auto txn_store = TxnManager::instance().get_txn_store();
        _status = cb(txn_store);
    }

private:
    Status _status;
};

} // namespace pain::common

#define PAIN_TXN(txn_store)                                                                                            \
    pain::common::TxnGuard(txn_store) + [&]([[maybe_unused]] pain::common::TxnStore * txn) mutable -> pain::Status
