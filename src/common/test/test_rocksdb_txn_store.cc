#include <gtest/gtest.h>
#include <pain/base/path.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <map>
#include <thread>
#include <tuple>
#include <vector>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include "common/rocksdb_txn_store.h"

// NOLINTBEGIN(readability-magic-numbers)

using namespace pain;
using namespace pain::common;

class TestRocksdbTxnStore : public testing::Test {
protected:
    void SetUp() override {
        std::string temp_dir = "/tmp/test_rocksdb_txn_XXXXXX";
        make_temp_dir_or_die(&temp_dir);
        _data_path = fmt::format("{}/test_rocksdb_txn", temp_dir);

        // 创建 RocksDB 数据库
        rocksdb::Options options;
        options.create_if_missing = true;
        options.error_if_exists = false;

        rocksdb::TransactionDBOptions txn_db_options;
        auto status = rocksdb::TransactionDB::Open(options, txn_db_options, _data_path, &_txn_db);
        ASSERT_TRUE(status.ok()) << status.ToString();
    }

    void TearDown() override {
        if (_txn_db != nullptr) {
            delete _txn_db;
            _txn_db = nullptr;
        }
        std::filesystem::remove_all(_data_path);
    }

    // 创建新的事务
    std::unique_ptr<RocksdbTxnStore> create_transaction() {
        auto txn = _txn_db->BeginTransaction(rocksdb::WriteOptions());
        BOOST_ASSERT(txn != nullptr);
        return std::make_unique<RocksdbTxnStore>(txn);
    }

    // 验证数据是否存在
    bool key_exists(const std::string& key) {
        std::string value;
        auto status = _txn_db->Get(rocksdb::ReadOptions(), key, &value);
        return status.ok();
    }

    // 获取数据值
    std::string get_value(const std::string& key) {
        std::string value;
        auto status = _txn_db->Get(rocksdb::ReadOptions(), key, &value);
        if (status.ok()) {
            return value;
        }
        return "";
    }

    const std::string& data_path() const {
        return _data_path;
    }

private:
    std::string _data_path;
    rocksdb::TransactionDB* _txn_db = nullptr;
};

// 测试构造函数和析构函数
TEST_F(TestRocksdbTxnStore, ConstructorAndDestructor) {
    auto txn = create_transaction();
    ASSERT_TRUE(txn != nullptr);

    // 测试析构函数不会崩溃
    txn.reset();
}

// 测试 hset 方法
TEST_F(TestRocksdbTxnStore, Hset) {
    auto txn = create_transaction();

    // 测试基本的 hset 操作
    auto status = txn->hset("test_key", "test_field", "test_value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据还没有提交到数据库
    std::string expected_key = "test_key\1test_field";
    ASSERT_FALSE(key_exists(expected_key));

    // 提交事务
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据已经提交到数据库
    ASSERT_TRUE(key_exists(expected_key));
    ASSERT_EQ(get_value(expected_key), "test_value");
}

// 测试 hset 多个字段
TEST_F(TestRocksdbTxnStore, HsetMultipleFields) {
    auto txn = create_transaction();

    // 设置多个字段
    auto status = txn->hset("user", "name", "luffy");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->hset("user", "age", "17");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->hset("user", "height", "175");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 提交事务
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证所有字段都已设置
    ASSERT_TRUE(key_exists("user\1name"));
    ASSERT_EQ(get_value("user\1name"), "luffy");

    ASSERT_TRUE(key_exists("user\1age"));
    ASSERT_EQ(get_value("user\1age"), "17");

    ASSERT_TRUE(key_exists("user\1height"));
    ASSERT_EQ(get_value("user\1height"), "175");
}

// 测试 hset 空值
TEST_F(TestRocksdbTxnStore, HsetEmptyValue) {
    auto txn = create_transaction();

    auto status = txn->hset("empty", "field", "");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    ASSERT_TRUE(key_exists("empty\1field"));
    ASSERT_EQ(get_value("empty\1field"), "");
}

// 测试 hset 特殊字符
TEST_F(TestRocksdbTxnStore, HsetSpecialCharacters) {
    auto txn = create_transaction();

    // 测试包含分隔符的 key 和 field
    auto status = txn->hset("key\1with\1separator", "field\1with\1separator", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    std::string expected_key = "key\1with\1separator\1field\1with\1separator";
    ASSERT_TRUE(key_exists(expected_key));
    ASSERT_EQ(get_value(expected_key), "value");
}

// 测试 hdel 方法
TEST_F(TestRocksdbTxnStore, Hdel) {
    auto txn = create_transaction();

    // 先设置一个值
    auto status = txn->hset("delete_test", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证值已设置
    ASSERT_TRUE(key_exists("delete_test\1field"));

    // 开始新事务删除值
    auto delete_txn = create_transaction();
    status = delete_txn->hdel("delete_test", "field");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证值还没有被删除
    ASSERT_TRUE(key_exists("delete_test\1field"));

    // 提交删除事务
    status = delete_txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证值已被删除
    ASSERT_FALSE(key_exists("delete_test\1field"));
}

// 测试删除不存在的字段
TEST_F(TestRocksdbTxnStore, HdelNonExistentField) {
    auto txn = create_transaction();

    // 删除不存在的字段应该成功（RocksDB 的 Delete 操作对不存在的 key 是幂等的）
    auto status = txn->hdel("non_existent", "field");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();
}

// 测试读取不存在的字段
TEST_F(TestRocksdbTxnStore, ReadNonExistentField) {
    auto txn = create_transaction();

    // 尝试读取不存在的字段应该失败
    std::string value;
    auto status = txn->hget("non_existent", "field", &value);
    ASSERT_FALSE(status.ok());

    // 检查不存在的字段应该返回 false
    bool exists = txn->hexists("non_existent", "field");
    ASSERT_FALSE(exists);

    // 获取不存在的哈希表长度应该返回 0
    size_t len = 0;
    status = txn->hlen("non_existent", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 0);

    // 获取不存在的哈希表的所有字段应该返回空迭代器
    auto iterator = txn->hgetall("non_existent");
    ASSERT_TRUE(iterator != nullptr);
    ASSERT_FALSE(iterator->valid());

    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();
}

// 测试 commit 方法
TEST_F(TestRocksdbTxnStore, Commit) {
    auto txn = create_transaction();

    auto status = txn->hset("commit_test", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 提交事务
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据已提交
    ASSERT_TRUE(key_exists("commit_test\1field"));
    ASSERT_EQ(get_value("commit_test\1field"), "value");
}

// 测试 rollback 方法
TEST_F(TestRocksdbTxnStore, Rollback) {
    auto txn = create_transaction();

    auto status = txn->hset("rollback_test", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 在回滚前，事务中应该能看到这个值
    std::string value;
    status = txn->hget("rollback_test", "field", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "value");

    // 回滚事务
    status = txn->rollback();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据没有提交到数据库
    ASSERT_FALSE(key_exists("rollback_test\1field"));
}

// 测试多次 commit 或 rollback
TEST_F(TestRocksdbTxnStore, MultipleCommitOrRollback) {
    auto txn = create_transaction();

    auto status = txn->hset("multi_test", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 第一次提交应该成功
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 第二次提交应该失败（事务已经结束）
    status = txn->commit();
    ASSERT_FALSE(status.ok());

    // 验证数据只提交了一次
    ASSERT_TRUE(key_exists("multi_test\1field"));
    ASSERT_EQ(get_value("multi_test\1field"), "value");
}

// 测试 hget 方法
TEST_F(TestRocksdbTxnStore, Hget) {
    auto txn = create_transaction();

    // 先设置一个值
    auto status = txn->hset("hget_test", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 在事务中读取值
    std::string value;
    status = txn->hget("hget_test", "field", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "value");

    // 提交事务
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据已提交到数据库
    ASSERT_TRUE(key_exists("hget_test\1field"));
    ASSERT_EQ(get_value("hget_test\1field"), "value");
}

// 测试 hlen 方法
TEST_F(TestRocksdbTxnStore, Hlen) {
    auto txn = create_transaction();

    // 设置多个字段
    auto status = txn->hset("hlen_test", "field1", "value1");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->hset("hlen_test", "field2", "value2");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->hset("hlen_test", "field3", "value3");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 在事务中获取长度
    size_t len = 0;
    status = txn->hlen("hlen_test", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 3);

    // 提交事务
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据已提交
    ASSERT_TRUE(key_exists("hlen_test\1field1"));
    ASSERT_TRUE(key_exists("hlen_test\1field2"));
    ASSERT_TRUE(key_exists("hlen_test\1field3"));
}

// 测试 hgetall 方法
TEST_F(TestRocksdbTxnStore, Hgetall) {
    auto txn = create_transaction();

    // 设置多个字段
    auto status = txn->hset("hgetall_test", "field1", "value1");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->hset("hgetall_test", "field2", "value2");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->hset("hgetall_test", "field3", "value3");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 在事务中获取所有字段
    auto iterator = txn->hgetall("hgetall_test");
    ASSERT_TRUE(iterator != nullptr);

    // 收集所有字段和值
    std::map<std::string, std::string> fields;
    while (iterator->valid()) {
        std::string key = std::string(iterator->key());
        std::string value = std::string(iterator->value());
        fields[key] = value;
        iterator->next();
    }

    // 验证字段数量
    ASSERT_EQ(fields.size(), 3);

    // 验证特定字段
    ASSERT_EQ(fields["hgetall_test\1field1"], "value1");
    ASSERT_EQ(fields["hgetall_test\1field2"], "value2");
    ASSERT_EQ(fields["hgetall_test\1field3"], "value3");

    // 提交事务
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据已提交
    ASSERT_TRUE(key_exists("hgetall_test\1field1"));
    ASSERT_TRUE(key_exists("hgetall_test\1field2"));
    ASSERT_TRUE(key_exists("hgetall_test\1field3"));
}

// 测试 hexists 方法
TEST_F(TestRocksdbTxnStore, Hexists) {
    auto txn = create_transaction();

    // 设置一个字段
    auto status = txn->hset("hexists_test", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 在事务中检查字段是否存在
    bool exists = txn->hexists("hexists_test", "field");
    ASSERT_TRUE(exists);

    // 检查不存在的字段
    exists = txn->hexists("hexists_test", "non_existent");
    ASSERT_FALSE(exists);

    // 提交事务
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据已提交
    ASSERT_TRUE(key_exists("hexists_test\1field"));
    ASSERT_EQ(get_value("hexists_test\1field"), "value");
}

// 测试 make_key 私有方法（通过公共接口间接测试）
TEST_F(TestRocksdbTxnStore, MakeKeyFormat) {
    auto txn = create_transaction();

    // 通过 hset 和 hdel 测试 key 格式
    auto status = txn->hset("prefix", "suffix", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证 key 格式：prefix\1suffix
    ASSERT_TRUE(key_exists("prefix\1suffix"));
    ASSERT_EQ(get_value("prefix\1suffix"), "value");
}

// 测试事务中的读取操作
TEST_F(TestRocksdbTxnStore, TransactionReadOperations) {
    auto txn = create_transaction();

    // 设置多个字段
    auto status = txn->hset("read_test", "field1", "value1");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->hset("read_test", "field2", "value2");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = txn->hset("read_test", "field3", "value3");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 在事务中读取单个字段
    std::string value;
    status = txn->hget("read_test", "field1", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "value1");

    status = txn->hget("read_test", "field2", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "value2");

    // 在事务中检查字段是否存在
    bool exists = txn->hexists("read_test", "field1");
    ASSERT_TRUE(exists);

    exists = txn->hexists("read_test", "non_existent");
    ASSERT_FALSE(exists);

    // 在事务中获取长度
    size_t len = 0;
    status = txn->hlen("read_test", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 3);

    // 在事务中获取所有字段
    auto iterator = txn->hgetall("read_test");
    ASSERT_TRUE(iterator != nullptr);

    std::map<std::string, std::string> fields;
    while (iterator->valid()) {
        std::string key = std::string(iterator->key());
        std::string val = std::string(iterator->value());
        fields[key] = val;
        iterator->next();
    }

    ASSERT_EQ(fields.size(), 3);
    ASSERT_EQ(fields["read_test\1field1"], "value1");
    ASSERT_EQ(fields["read_test\1field2"], "value2");
    ASSERT_EQ(fields["read_test\1field3"], "value3");

    // 提交事务
    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据已提交到数据库
    ASSERT_TRUE(key_exists("read_test\1field1"));
    ASSERT_TRUE(key_exists("read_test\1field2"));
    ASSERT_TRUE(key_exists("read_test\1field3"));
}

// 测试事务隔离性
TEST_F(TestRocksdbTxnStore, TransactionIsolation) {
    // 第一个事务设置值
    auto txn1 = create_transaction();
    auto status = txn1->hset("isolation_test", "field", "value1");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 第二个事务读取相同的 key（应该看不到第一个事务的修改）
    auto txn2 = create_transaction();

    // 在第二个事务中读取，应该看不到第一个事务的修改
    std::string value;
    status = txn2->hget("isolation_test", "field", &value);
    ASSERT_FALSE(status.ok()); // 应该失败，因为字段还不存在

    // 第二个事务设置不同的值
    auto f = std::async(std::launch::async, [&]() {
        auto status = txn2->hset("isolation_test", "field", "value2");
        return status;
    });

    // 提交第一个事务
    status = txn1->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证第一个事务的值
    ASSERT_TRUE(key_exists("isolation_test\1field"));
    ASSERT_EQ(get_value("isolation_test\1field"), "value1");

    status = f.get();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 提交第二个事务（应该覆盖第一个事务的值）
    status = txn2->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证第二个事务的值覆盖了第一个
    ASSERT_TRUE(key_exists("isolation_test\1field"));
    ASSERT_EQ(get_value("isolation_test\1field"), "value2");
}

// 测试并发事务
TEST_F(TestRocksdbTxnStore, ConcurrentTransactions) {
    const int num_transactions = 10;
    std::vector<std::unique_ptr<RocksdbTxnStore>> transactions;
    std::vector<std::thread> threads;

    // 创建多个事务
    for (int i = 0; i < num_transactions; ++i) {
        transactions.push_back(create_transaction());
    }

    // 在每个事务中设置不同的值
    for (int i = 0; i < num_transactions; ++i) {
        auto status = transactions[i]->hset("concurrent_test", fmt::format("field{}", i), fmt::format("value{}", i));
        ASSERT_TRUE(status.ok()) << status.error_str();
    }

    // 并发提交所有事务
    for (int i = 0; i < num_transactions; ++i) {
        auto status = transactions[i]->commit();
        ASSERT_TRUE(status.ok()) << status.error_str();
    }

    // 验证所有值都已设置
    for (int i = 0; i < num_transactions; ++i) {
        std::string key = fmt::format("concurrent_test\1field{}", i);
        ASSERT_TRUE(key_exists(key));
        ASSERT_EQ(get_value(key), fmt::format("value{}", i));
    }
}

// 测试错误处理
TEST_F(TestRocksdbTxnStore, ErrorHandling) {
    // 测试无效的 key 或 field（空字符串）
    auto txn = create_transaction();
    auto status = txn->hset("", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str(); // RocksDB 允许空 key

    status = txn->hset("key", "", "value");
    ASSERT_TRUE(status.ok()) << status.error_str(); // RocksDB 允许空 field

    status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证空 key 和 field 也能正常工作
    ASSERT_TRUE(key_exists("\1field"));
    ASSERT_EQ(get_value("\1field"), "value");

    ASSERT_TRUE(key_exists("key\1"));
    ASSERT_EQ(get_value("key\1"), "value");
}

// 性能测试
TEST_F(TestRocksdbTxnStore, PerformanceTest) {
    const int num_operations = 1000;
    auto txn = create_transaction();

    auto start_time = std::chrono::high_resolution_clock::now();

    // 批量设置值
    for (int i = 0; i < num_operations; ++i) {
        auto status = txn->hset("perf_test", fmt::format("field{}", i), fmt::format("value{}", i));
        ASSERT_TRUE(status.ok()) << status.error_str();
    }

    auto set_end_time = std::chrono::high_resolution_clock::now();

    // 提交事务
    auto status = txn->commit();
    ASSERT_TRUE(status.ok()) << status.error_str();

    auto commit_end_time = std::chrono::high_resolution_clock::now();

    // 计算性能指标
    auto set_duration = std::chrono::duration_cast<std::chrono::microseconds>(set_end_time - start_time);
    auto commit_duration = std::chrono::duration_cast<std::chrono::microseconds>(commit_end_time - set_end_time);

    // 验证所有操作都成功
    for (int i = 0; i < num_operations; ++i) {
        std::string key = fmt::format("perf_test\1field{}", i);
        ASSERT_TRUE(key_exists(key));
        ASSERT_EQ(get_value(key), fmt::format("value{}", i));
    }

    // 输出性能信息（可选）
    std::cout << "Performance test results:" << std::endl;
    std::cout << "  Set operations: " << num_operations << " in " << set_duration.count() << " microseconds"
              << std::endl;
    std::cout << "  Commit time: " << commit_duration.count() << " microseconds" << std::endl;
    std::cout << "  Average set time per operation: " << (set_duration.count() / num_operations) << " microseconds"
              << std::endl;
}

// NOLINTEND(readability-magic-numbers)
