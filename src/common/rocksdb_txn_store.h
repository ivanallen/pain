#pragma once

#include <rocksdb/options.h>
#include "common/store.h"
#include "common/txn_store.h"

namespace rocksdb {
class Transaction;
}

namespace pain::common {

class RocksdbTxnStore : public TxnStore {
public:
    RocksdbTxnStore(rocksdb::Transaction* txn);
    ~RocksdbTxnStore() override;

    Status commit() override;
    Status rollback() override;

    Status hset(std::string_view key, std::string_view field, std::string_view value) override;
    Status hget(std::string_view key, std::string_view field, std::string* value) override;
    Status hdel(std::string_view key, std::string_view field) override;
    Status hlen(std::string_view key, size_t* len) override;
    std::shared_ptr<Iterator> hgetall(std::string_view key) override;
    bool hexists(std::string_view key, std::string_view field) override;

private:
    std::string make_key(std::string_view key, std::string_view field) const;
    rocksdb::Transaction* _txn;
    rocksdb::ReadOptions _read_options;
};

} // namespace pain::common
