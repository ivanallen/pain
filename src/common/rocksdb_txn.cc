#include "common/rocksdb_txn.h"
#include <pain/base/plog.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>

#define PAIN_COMMON_ROCKSDB_TXN_KEY_VALUE_SEPARATOR "\1"

namespace pain::common {

class RocksdbTxnIterator : public Store::Iterator {
public:
    RocksdbTxnIterator(rocksdb::Iterator* iter, std::string_view prefix) :
        _iter(iter),
        _prefix(fmt::format("{}" PAIN_COMMON_ROCKSDB_TXN_KEY_VALUE_SEPARATOR, prefix)) {}
    ~RocksdbTxnIterator() override {
        delete _iter;
    }

    bool valid() override {
        return _iter->Valid() && _iter->key().starts_with(_prefix);
    }

    std::string_view key() override {
        return std::string_view(_iter->key().data(), _iter->key().size());
    }

    std::string_view value() override {
        return std::string_view(_iter->value().data(), _iter->value().size());
    }

    void next() override {
        _iter->Next();
    }

private:
    rocksdb::Iterator* _iter;
    std::string _prefix;
};

RocksdbTxn::RocksdbTxn(rocksdb::Transaction* txn) : _txn(txn) {}

RocksdbTxn::~RocksdbTxn() {
    delete _txn;
}

Status RocksdbTxn::commit() {
    auto status = _txn->Commit();
    if (!status.ok()) {
        return Status(EIO, status.ToString());
    }
    return Status::OK();
}

Status RocksdbTxn::rollback() {
    auto status = _txn->Rollback();
    if (!status.ok()) {
        return Status(EIO, status.ToString());
    }
    return Status::OK();
}

std::string RocksdbTxn::make_key(std::string_view key, std::string_view field) const {
    return std::format("{}" PAIN_COMMON_ROCKSDB_TXN_KEY_VALUE_SEPARATOR "{}", key, field);
}

Status RocksdbTxn::hset(std::string_view key, std::string_view field, std::string_view value) {
    auto status = _txn->Put(make_key(key, field), value);
    if (!status.ok()) {
        return Status(EIO, status.ToString());
    }
    return Status::OK();
}

Status RocksdbTxn::hget(std::string_view key, std::string_view field, std::string* value) {
    rocksdb::Status status = _txn->Get(_read_options, make_key(key, field), value);
    if (!status.ok()) {
        PLOG_ERROR(("desc", "hget failed") //
                   ("key", key)("field", field)("error", status.ToString()));
        return Status(EIO, status.ToString());
    }
    return Status::OK();
}

Status RocksdbTxn::hdel(std::string_view key, std::string_view field) {
    auto status = _txn->Delete(make_key(key, field));
    if (!status.ok()) {
        return Status(EIO, status.ToString());
    }
    return Status::OK();
}

Status RocksdbTxn::hlen(std::string_view key, size_t* len) {
    auto it = hgetall(key);
    size_t size = 0;
    while (it->valid()) {
        it->next();
        ++size;
    }
    *len = size;
    return Status::OK();
}

std::shared_ptr<Store::Iterator> RocksdbTxn::hgetall(std::string_view key) {
    rocksdb::Iterator* iter = _txn->GetIterator(_read_options);
    iter->Seek(key);
    return std::make_shared<RocksdbTxnIterator>(iter, key);
}

bool RocksdbTxn::hexists(std::string_view key, std::string_view field) {
    std::string value;
    auto status = hget(key, field, &value);
    return status.ok();
}

} // namespace pain::common

#undef PAIN_COMMON_ROCKSDB_TXN_KEY_VALUE_SEPARATOR
