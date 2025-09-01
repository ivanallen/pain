#include "common/rocksdb_store.h"
#include <braft/file_system_adaptor.h>
#include <pain/base/plog.h>
#include <pain/base/scope_exit.h>
#include <pain/base/types.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/checkpoint.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <string>
#include <boost/assert.hpp>
#include "common/rocksdb_txn_store.h"
#include "common/rocksdb_util.h"

#define PAIN_COMMON_ROCKSDB_STORE_KEY_VALUE_SEPARATOR "\1"

namespace pain::common {

RocksdbStore::RocksdbStore() {
    _write_options.disableWAL = true;
    _write_options.sync = false;
}

RocksdbStore::~RocksdbStore() {
    auto status = close();
    if (!status.ok()) {
        PLOG_ERROR(("desc", "close rocksdb failed") //
                   ("error", status.error_str()));
    }
}

Status RocksdbStore::open(const char* data_path, RocksdbStorePtr* store) {
    BOOST_ASSERT(data_path != nullptr);
    BOOST_ASSERT(store != nullptr);
    auto fs = braft::default_file_system();

    if (!fs->directory_exists(data_path)) {
        butil::File::Error error = butil::File::FILE_OK;
        if (!fs->create_directory(data_path, &error, true)) {
            if (error != butil::File::FILE_OK) {
                PLOG_ERROR(("desc", "create dir failed") //
                           ("path", data_path)           //
                           ("error", static_cast<int>(error)));
            }
            return Status(EIO, "create dir %s failed", data_path);
        }
    }
    rocksdb::Options options;
    rocksdb::TransactionDBOptions txn_options;
    options.create_if_missing = true;
    PLOG_INFO(("desc", "open rocksdb") //
              ("path", data_path));

    rocksdb::TransactionDB* txn_db = nullptr;
    rocksdb::Status status = rocksdb::TransactionDB::Open(options, txn_options, data_path, &txn_db);
    if (!status.ok()) {
        PLOG_ERROR(("desc", "open rocksdb failed") //
                   ("path", data_path)("error", status.ToString()));
        return convert_to_pain_status(status);
    }

    RocksdbStorePtr rocksdb_store = new RocksdbStore();
    rocksdb_store->_data_path = data_path;
    rocksdb_store->_db = txn_db->GetBaseDB();
    rocksdb_store->_txn_db = txn_db;
    *store = rocksdb_store;
    return Status::OK();
}

void RocksdbStore::open_or_die() {
    rocksdb::Options options;
    rocksdb::TransactionDBOptions txn_options;
    rocksdb::Status status = rocksdb::TransactionDB::Open(options, txn_options, _data_path, &_txn_db);
    if (!status.ok()) {
        PLOG_ERROR(("desc", "open rocksdb failed") //
                   ("path", _data_path)("error", status.ToString()));
        BOOST_ASSERT_MSG(false, status.ToString().c_str());
    }
    _db = _txn_db->GetBaseDB();
}

Status RocksdbStore::close() {
    if (_db == nullptr) {
        return Status::OK();
    }
    PLOG_INFO(("desc", "close rocksdb") //
              ("path", _data_path));
    auto status = _db->Close();
    if (!status.ok()) {
        PLOG_ERROR(("desc", "close rocksdb failed") //
                   ("path", _data_path)("error", status.ToString()));
        return convert_to_pain_status(status);
    }
    delete _txn_db;
    _txn_db = nullptr;
    _db = nullptr;
    return Status::OK();
}

Status RocksdbStore::check_point(const char* to, std::vector<std::string>* files) {
    BOOST_ASSERT(to != nullptr);
    BOOST_ASSERT(files != nullptr);
    rocksdb::FlushOptions options;
    options.wait = true;
    options.allow_write_stall = false;
    auto st = _db->Flush(options);
    if (!st.ok()) {
        PLOG_ERROR(("desc", "flush rocksdb failed") //
                   ("error", st.ToString()));
        return convert_to_pain_status(st);
    }

    rocksdb::Checkpoint* cpt = nullptr;
    rocksdb::Status status = rocksdb::Checkpoint::Create(_db, &cpt);
    if (!status.ok()) {
        PLOG_ERROR(("desc", "create checkpoint failed") //
                   ("error", status.ToString()));
        return convert_to_pain_status(status);
    }
    std::unique_ptr<rocksdb::Checkpoint> cpt_guard(cpt);

    status = cpt->CreateCheckpoint(to);
    if (!status.ok()) {
        PLOG_ERROR(("desc", "create checkpoint failed") //
                   ("error", status.ToString()));
        return convert_to_pain_status(status);
    }

    auto fs = braft::default_file_system();
    std::unique_ptr<braft::DirReader> dir_reader(fs->directory_reader(to));

    if (dir_reader == nullptr) {
        PLOG_ERROR(("desc", "fail to open cpt dir") //
                   ("path", to));
        BOOST_ASSERT_MSG(false, "fail to open cpt dir");
    }

    if (!dir_reader->is_valid()) {
        PLOG_ERROR(("desc", "cpt dir is invalid") //
                   ("path", to));
        BOOST_ASSERT_MSG(false, "cpt dir is invalid");
    }

    std::vector<std::string> snapshot_files;
    while (dir_reader->next()) {
        auto file_name = fmt::format("cpt/{}", dir_reader->name());
        PLOG_INFO(("desc", "snapshot add file") //
                  ("file", file_name));
        snapshot_files.push_back(file_name);
    }

    files->swap(snapshot_files);
    return Status::OK();
}

Status RocksdbStore::recover(const char* from) {
    BOOST_ASSERT(from != nullptr);
    PLOG_INFO(("desc", "recover rocksdb") //
              ("from", from));
    auto status = close();
    if (!status.ok()) {
        return status;
    }

    auto rollback_open = make_scope_exit([&] {
        open_or_die();
    });

    auto fs = braft::default_file_system();
    auto bak_path = _data_path + ".bak";
    if (fs->path_exists(bak_path) && !fs->delete_file(bak_path, true)) {
        PLOG_ERROR(("desc", "delete bak failed") //
                   ("path", bak_path));
        return Status(EIO, "remove bak dir failed");
    }
    if (!fs->rename(_data_path, bak_path)) {
        PLOG_ERROR(("desc", "rename failed") //
                   ("src", _data_path)       //
                   ("dest", bak_path));
        return Status(EIO, "rename failed");
    }

    auto rollback_dir = make_scope_exit([&] {
        if (!fs->rename(bak_path, _data_path)) {
            PLOG_ERROR(("desc", "rollback failed") //
                       ("src", bak_path)           //
                       ("dest", _data_path));
            BOOST_ASSERT_MSG(false, "rollbak rename failed");
        }
    });

    rocksdb::Options options;
    rocksdb::DB* db = nullptr;
    rocksdb::Status st = rocksdb::DB::OpenForReadOnly(options, from, &db);
    std::unique_ptr<rocksdb::DB> db_guard(db);

    if (!st.ok()) {
        PLOG_ERROR(("desc", "recover rocksdb failed") //
                   ("from", from)("error", st.ToString()));
        return convert_to_pain_status(st);
    }

    rocksdb::Checkpoint* cpt = nullptr;
    st = rocksdb::Checkpoint::Create(db, &cpt);
    if (!st.ok()) {
        PLOG_ERROR(("desc", "create checkpoint failed") //
                   ("error", st.ToString()));
        return convert_to_pain_status(st);
    }
    std::unique_ptr<rocksdb::Checkpoint> cpt_guard(cpt);

    st = cpt->CreateCheckpoint(_data_path);
    if (!st.ok()) {
        PLOG_ERROR(("desc", "create checkpoint failed") //
                   ("error", st.ToString()));
        return convert_to_pain_status(st);
    }

    rollback_dir.release();
    rollback_open.release();

    if (!fs->delete_file(bak_path, true)) {
        // Only log here.
        PLOG_ERROR(("desc", "delete bak failed") //
                   ("path", bak_path));
    }

    open_or_die();

    return Status::OK();
}

std::string RocksdbStore::make_key(std::string_view key, std::string_view field) const {
    return std::format("{}" PAIN_COMMON_ROCKSDB_STORE_KEY_VALUE_SEPARATOR "{}", key, field);
}

Status RocksdbStore::hset(std::string_view key, std::string_view field, std::string_view value) {
    rocksdb::Status status = _db->Put(_write_options, make_key(key, field), value);
    if (!status.ok()) {
        PLOG_ERROR(("desc", "hset failed") //
                   ("key", key)("field", field)("error", status.ToString()));
        return convert_to_pain_status(status);
    }
    return Status::OK();
}

Status RocksdbStore::hget(std::string_view key, std::string_view field, std::string* value) {
    rocksdb::Status status = _db->Get(_read_options, make_key(key, field), value);
    if (!status.ok()) {
        PLOG_ERROR(("desc", "hget failed") //
                   ("key", key)("field", field)("error", status.ToString()));
        return convert_to_pain_status(status);
    }
    return Status::OK();
}

Status RocksdbStore::hdel(std::string_view key, std::string_view field) {
    rocksdb::Status status = _db->Delete(_write_options, make_key(key, field));
    if (!status.ok()) {
        PLOG_ERROR(("desc", "hdel failed") //
                   ("key", key)("field", field)("error", status.ToString()));
        return convert_to_pain_status(status);
    }
    return Status::OK();
}

Status RocksdbStore::hlen(std::string_view key, size_t* len) {
    auto it = hgetall(key);
    size_t size = 0;
    while (it->valid()) {
        it->next();
        ++size;
    }
    *len = size;
    return Status::OK();
}

class RocksdbStoreIterator : public Store::Iterator {
public:
    RocksdbStoreIterator(rocksdb::Iterator* iter, std::string_view prefix) :
        _iter(iter),
        _prefix(fmt::format("{}" PAIN_COMMON_ROCKSDB_STORE_KEY_VALUE_SEPARATOR, prefix)) {}
    ~RocksdbStoreIterator() override {
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

std::shared_ptr<RocksdbStore::Iterator> RocksdbStore::hgetall(std::string_view key) {
    rocksdb::Iterator* iter = _db->NewIterator(_read_options);
    iter->Seek(key);
    return std::make_shared<RocksdbStoreIterator>(iter, key);
}

bool RocksdbStore::hexists(std::string_view key, std::string_view field) {
    std::string value;
    auto status = hget(key, field, &value);
    return status.ok();
}

std::shared_ptr<TxnStore> RocksdbStore::begin_txn() {
    rocksdb::Transaction* txn = _txn_db->BeginTransaction(_write_options);
    return std::make_shared<RocksdbTxnStore>(txn);
}

} // namespace pain::common

#undef PAIN_COMMON_ROCKSDB_STORE_KEY_VALUE_SEPARATOR
