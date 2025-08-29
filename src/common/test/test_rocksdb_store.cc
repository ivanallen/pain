#include <gtest/gtest.h>
#include <pain/base/path.h>
#include <chrono>
#include <filesystem>
#include <map>
#include <tuple>
#include <vector>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include "common/rocksdb_store.h"

// NOLINTBEGIN(readability-magic-numbers)

using namespace pain;
using namespace pain::common;

class RocksdbStoreTest : public testing::Test {
protected:
    void SetUp() override {
        std::string temp_dir = "/tmp/test_rocksdb_store_XXXXXX";
        make_temp_dir_or_die(&temp_dir);
        _data_path = fmt::format("{}/test_rocksdb", temp_dir);
        _cpt_path = fmt::format("{}/test_rocksdb_cpt", temp_dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(data_path());
        std::filesystem::remove_all(cpt_path());
    }

    void create_data(RocksdbStorePtr store) {
        auto status = store->hset("pain", "name", "luffy");
        ASSERT_TRUE(status.ok()) << status.error_str();
        status = store->hset("pain", "age", "17");
        ASSERT_TRUE(status.ok()) << status.error_str();
        status = store->hset("pain", "height", "175");
        ASSERT_TRUE(status.ok()) << status.error_str();
        status = store->hset("pain", "weight", "60");
        ASSERT_TRUE(status.ok()) << status.error_str();

        status = store->hset("deva", "name", "zoro");
        ASSERT_TRUE(status.ok()) << status.error_str();
        status = store->hset("deva", "age", "19");
        ASSERT_TRUE(status.ok()) << status.error_str();
        status = store->hset("deva", "height", "180");
        ASSERT_TRUE(status.ok()) << status.error_str();
        status = store->hset("deva", "weight", "70");
        ASSERT_TRUE(status.ok()) << status.error_str();
    }

    void check(RocksdbStorePtr store) {
        std::string value;
        auto status = store->hget("pain", "name", &value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(value, "luffy");
        status = store->hget("pain", "age", &value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(value, "17");
        status = store->hget("pain", "height", &value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(value, "175");
        status = store->hget("pain", "weight", &value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(value, "60");

        status = store->hget("deva", "name", &value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(value, "zoro");
        status = store->hget("deva", "age", &value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(value, "19");
        status = store->hget("deva", "height", &value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(value, "180");
        status = store->hget("deva", "weight", &value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(value, "70");

        size_t len = 0;
        status = store->hlen("pain", &len);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(len, 4);
        status = store->hlen("deva", &len);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(len, 4);
    }

    const std::string& data_path() const {
        return _data_path;
    }

    const std::string& cpt_path() const {
        return _cpt_path;
    }

private:
    std::string _data_path;
    std::string _cpt_path;
};

TEST_F(RocksdbStoreTest, open) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();
    store->close();
}

TEST_F(RocksdbStoreTest, hsetget) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();
    create_data(store);
    check(store);
    store->close();
}

TEST_F(RocksdbStoreTest, hexists) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();
    create_data(store);
    check(store);
    bool exists = store->hexists("pain", "name");
    ASSERT_TRUE(exists);
    exists = store->hexists("pain", "not_exists");
    ASSERT_FALSE(exists);
    store->close();
}

TEST_F(RocksdbStoreTest, check_point) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();
    create_data(store);
    check(store);
    std::vector<std::string> files;
    status = store->check_point(cpt_path().c_str(), &files);
    ASSERT_TRUE(status.ok()) << status.error_str();
    fmt::println("{}", fmt::join(files, ", "));
    check(store);

    status = store->hset("sad", "name", "sanji");
    EXPECT_TRUE(status.ok()) << status.error_str();
    status = store->hset("sad", "age", "20");
    EXPECT_TRUE(status.ok()) << status.error_str();
    status = store->hset("sad", "height", "180");
    EXPECT_TRUE(status.ok()) << status.error_str();
    status = store->hset("sad", "weight", "70");
    EXPECT_TRUE(status.ok()) << status.error_str();

    size_t len = 0;
    status = store->hlen("sad", &len);
    EXPECT_TRUE(status.ok()) << status.error_str();

    std::string value;
    status = store->hget("sad", "name", &value);
    EXPECT_TRUE(status.ok()) << status.error_str();
    EXPECT_EQ(value, "sanji");
    status = store->hget("sad", "age", &value);
    EXPECT_TRUE(status.ok()) << status.error_str();
    EXPECT_EQ(value, "20");
    status = store->hget("sad", "height", &value);
    EXPECT_TRUE(status.ok()) << status.error_str();
    EXPECT_EQ(value, "180");
    status = store->hget("sad", "weight", &value);
    EXPECT_TRUE(status.ok()) << status.error_str();
    EXPECT_EQ(value, "70");

    status = store->recover(cpt_path().c_str());
    EXPECT_TRUE(status.ok()) << status.error_str();

    len = 0;
    status = store->hlen("sad", &len);
    EXPECT_TRUE(status.ok()) << status.error_str();
    EXPECT_EQ(len, 0);

    store->close();
}

TEST_F(RocksdbStoreTest, hdel) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 先创建数据
    status = store->hset("test", "field1", "value1");
    ASSERT_TRUE(status.ok()) << status.error_str();
    status = store->hset("test", "field2", "value2");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证字段存在
    ASSERT_TRUE(store->hexists("test", "field1"));
    ASSERT_TRUE(store->hexists("test", "field2"));

    // 删除字段
    status = store->hdel("test", "field1");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证字段已被删除
    ASSERT_FALSE(store->hexists("test", "field1"));
    ASSERT_TRUE(store->hexists("test", "field2"));

    // 验证长度变化
    size_t len = 0;
    status = store->hlen("test", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 1);

    store->close();
}

TEST_F(RocksdbStoreTest, hgetall) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 创建测试数据
    status = store->hset("user", "name", "luffy");
    ASSERT_TRUE(status.ok()) << status.error_str();
    status = store->hset("user", "age", "17");
    ASSERT_TRUE(status.ok()) << status.error_str();
    status = store->hset("user", "occupation", "pirate");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 获取所有字段
    auto iterator = store->hgetall("user");
    ASSERT_TRUE(iterator != nullptr);

    std::map<std::string, std::string> fields;
    while (iterator->valid()) {
        std::string key = std::string(iterator->key());
        std::string value = std::string(iterator->value());
        fields[key] = value;
        iterator->next();
    }

    // 验证所有字段都被正确获取
    ASSERT_EQ(fields.size(), 3);
    ASSERT_EQ(fields[store->make_key("user", "name")], "luffy");
    ASSERT_EQ(fields[store->make_key("user", "age")], "17");
    ASSERT_EQ(fields[store->make_key("user", "occupation")], "pirate");

    iterator.reset();

    store->close();
}

TEST_F(RocksdbStoreTest, hgetall_empty_key) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 获取不存在的key的所有字段
    auto iterator = store->hgetall("non_existent");
    ASSERT_TRUE(iterator != nullptr);

    // 迭代器应该无效
    ASSERT_FALSE(iterator->valid());
    iterator.reset();

    store->close();
}

TEST_F(RocksdbStoreTest, hlen_edge_cases) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 测试空key的长度
    size_t len = 0;
    status = store->hlen("empty_key", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 0);

    // 测试只有一个字段的key
    status = store->hset("single", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = store->hlen("single", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 1);

    // 删除字段后测试长度
    status = store->hdel("single", "field");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = store->hlen("single", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 0);

    store->close();
}

TEST_F(RocksdbStoreTest, multiple_operations) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 批量操作
    std::vector<std::tuple<std::string, std::string, std::string>> test_data = {{"batch1", "field1", "value1"},
                                                                                {"batch1", "field2", "value2"},
                                                                                {"batch2", "field1", "value3"},
                                                                                {"batch2", "field2", "value4"},
                                                                                {"batch2", "field3", "value5"}};

    // 批量插入
    for (const auto& [key, field, value] : test_data) {
        status = store->hset(key, field, value);
        ASSERT_TRUE(status.ok()) << status.error_str();
    }

    // 验证数据
    for (const auto& [key, field, value] : test_data) {
        std::string retrieved_value;
        status = store->hget(key, field, &retrieved_value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(retrieved_value, value);
    }

    // 验证长度
    size_t len = 0;
    status = store->hlen("batch1", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 2);

    status = store->hlen("batch2", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 3);

    store->close();
}

TEST_F(RocksdbStoreTest, update_existing_field) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 设置初始值
    status = store->hset("update_test", "field", "old_value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证初始值
    std::string value;
    status = store->hget("update_test", "field", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "old_value");

    // 更新值
    status = store->hset("update_test", "field", "new_value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证更新后的值
    status = store->hget("update_test", "field", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "new_value");

    // 长度应该保持不变
    size_t len = 0;
    status = store->hlen("update_test", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 1);

    store->close();
}

TEST_F(RocksdbStoreTest, delete_nonexistent_field) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 删除不存在的字段
    status = store->hdel("test_key", "nonexistent_field");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证字段确实不存在
    ASSERT_FALSE(store->hexists("test_key", "nonexistent_field"));

    // 长度应该为0
    size_t len = 0;
    status = store->hlen("test_key", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 0);

    store->close();
}

TEST_F(RocksdbStoreTest, get_nonexistent_field) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 尝试获取不存在的字段
    std::string value;
    status = store->hget("test_key", "nonexistent_field", &value);
    ASSERT_FALSE(status.ok());

    store->close();
}

TEST_F(RocksdbStoreTest, empty_string_values) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 设置空字符串值
    status = store->hset("empty_test", "empty_field", "");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证空字符串值
    std::string value;
    status = store->hget("empty_test", "empty_field", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "");

    // 验证字段存在
    ASSERT_TRUE(store->hexists("empty_test", "empty_field"));

    // 验证长度
    size_t len = 0;
    status = store->hlen("empty_test", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 1);

    store->close();
}

TEST_F(RocksdbStoreTest, large_value) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 创建大字符串值
    std::string large_value(10000, 'A');
    status = store->hset("large_test", "large_field", large_value);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证大字符串值
    std::string retrieved_value;
    status = store->hget("large_test", "large_field", &retrieved_value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(retrieved_value, large_value);
    ASSERT_EQ(retrieved_value.size(), 10000);

    store->close();
}

TEST_F(RocksdbStoreTest, special_characters) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 测试包含特殊字符的key和field
    std::string special_key = "key/with\\special:chars";
    std::string special_field = "field/with\\special:chars";
    std::string special_value = "value/with\\special:chars";

    status = store->hset(special_key, special_field, special_value);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证特殊字符
    std::string retrieved_value;
    status = store->hget(special_key, special_field, &retrieved_value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(retrieved_value, special_value);

    // 验证字段存在
    ASSERT_TRUE(store->hexists(special_key, special_field));

    store->close();
}

TEST_F(RocksdbStoreTest, concurrent_access_simulation) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 模拟并发访问场景
    for (int i = 0; i < 100; ++i) {
        std::string key = fmt::format("concurrent_key_{}", i);
        std::string field = fmt::format("field_{}", i);
        std::string value = fmt::format("value_{}", i);

        status = store->hset(key, field, value);
        ASSERT_TRUE(status.ok()) << status.error_str();

        // 立即验证
        std::string retrieved_value;
        status = store->hget(key, field, &retrieved_value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(retrieved_value, value);

        // 验证存在性
        ASSERT_TRUE(store->hexists(key, field));
    }

    // 验证总数
    for (int i = 0; i < 100; ++i) {
        size_t len = 0;
        std::string key = fmt::format("concurrent_key_{}", i);
        status = store->hlen(key, &len);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(len, 1) << key;
    }

    store->close();
}

TEST_F(RocksdbStoreTest, stress_test) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    const int num_keys = 1000;
    const int num_fields_per_key = 10;

    // 创建大量数据
    for (int i = 0; i < num_keys; ++i) {
        for (int j = 0; j < num_fields_per_key; ++j) {
            std::string key = fmt::format("stress_key_{}", i);
            std::string field = fmt::format("field_{}", j);
            std::string value = fmt::format("value_{}_{}", i, j);

            status = store->hset(key, field, value);
            ASSERT_TRUE(status.ok()) << status.error_str();
        }
    }

    // 验证所有数据
    for (int i = 0; i < num_keys; ++i) {
        std::string key = fmt::format("stress_key_{}", i);
        size_t len = 0;
        status = store->hlen(key, &len);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(len, num_fields_per_key);

        // 随机验证几个字段
        for (int j = 0; j < 3; ++j) {
            std::string field = fmt::format("field_{}", j);
            std::string expected_value = fmt::format("value_{}_{}", i, j);
            std::string actual_value;

            status = store->hget(key, field, &actual_value);
            ASSERT_TRUE(status.ok()) << status.error_str();
            ASSERT_EQ(actual_value, expected_value);
        }
    }

    store->close();
}

TEST_F(RocksdbStoreTest, mixed_operations) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 混合操作：插入、更新、删除
    std::string key = "mixed_test";

    // 插入字段
    status = store->hset(key, "field1", "value1");
    ASSERT_TRUE(status.ok()) << status.error_str();
    status = store->hset(key, "field2", "value2");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证长度
    size_t len = 0;
    status = store->hlen(key, &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 2);

    // 更新字段
    status = store->hset(key, "field1", "updated_value1");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证更新
    std::string value;
    status = store->hget(key, "field1", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "updated_value1");

    // 删除字段
    status = store->hdel(key, "field2");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证删除
    ASSERT_FALSE(store->hexists(key, "field2"));

    // 验证最终长度
    status = store->hlen(key, &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 1);

    store->close();
}

TEST_F(RocksdbStoreTest, iterator_behavior) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 创建测试数据
    std::vector<std::pair<std::string, std::string>> test_data = {
        {"field1", "value1"}, {"field2", "value2"}, {"field3", "value3"}};

    for (const auto& [field, value] : test_data) {
        status = store->hset("iterator_test", field, value);
        ASSERT_TRUE(status.ok()) << status.error_str();
    }

    // 测试迭代器
    auto iterator = store->hgetall("iterator_test");
    ASSERT_TRUE(iterator != nullptr);

    // 验证迭代器初始状态
    ASSERT_TRUE(iterator->valid());

    // 收集所有数据
    std::vector<std::pair<std::string, std::string>> collected_data;
    while (iterator->valid()) {
        std::string key = std::string(iterator->key());
        std::string value = std::string(iterator->value());
        collected_data.emplace_back(key, value);
        iterator->next();
    }

    // 验证收集的数据
    ASSERT_EQ(collected_data.size(), 3);

    // 验证迭代器最终状态
    ASSERT_FALSE(iterator->valid());

    iterator.reset();
    store->close();
}

TEST_F(RocksdbStoreTest, key_field_naming_convention) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 测试不同的命名约定
    std::vector<std::string> keys = {"user:123", "product:456", "order:789", "category:electronics"};

    std::vector<std::string> fields = {"name", "price", "status", "created_at"};

    // 插入数据
    for (const auto& key : keys) {
        for (const auto& field : fields) {
            std::string value = fmt::format("{}_{}_value", key, field);
            status = store->hset(key, field, value);
            ASSERT_TRUE(status.ok()) << status.error_str();
        }
    }

    // 验证数据
    for (const auto& key : keys) {
        size_t len = 0;
        status = store->hlen(key, &len);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(len, fields.size());

        for (const auto& field : fields) {
            std::string expected_value = fmt::format("{}_{}_value", key, field);
            std::string actual_value;

            status = store->hget(key, field, &actual_value);
            ASSERT_TRUE(status.ok()) << status.error_str();
            ASSERT_EQ(actual_value, expected_value);
        }
    }

    store->close();
}

TEST_F(RocksdbStoreTest, data_persistence) {
    RocksdbStorePtr store1;
    RocksdbStorePtr store2;

    // 第一个store写入数据
    auto status = RocksdbStore::open(data_path().c_str(), &store1);
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = store1->hset("persistence_test", "field1", "value1");
    ASSERT_TRUE(status.ok()) << status.error_str();
    status = store1->hset("persistence_test", "field2", "value2");
    ASSERT_TRUE(status.ok()) << status.error_str();

    store1->close();

    // 第二个store读取数据
    status = RocksdbStore::open(data_path().c_str(), &store2);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证数据持久化
    std::string value;
    status = store2->hget("persistence_test", "field1", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "value1");

    status = store2->hget("persistence_test", "field2", &value);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(value, "value2");

    size_t len = 0;
    status = store2->hlen("persistence_test", &len);
    ASSERT_TRUE(status.ok()) << status.error_str();
    ASSERT_EQ(len, 2);

    store2->close();
}

TEST_F(RocksdbStoreTest, error_handling) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 测试空字符串key和field
    status = store->hset("", "field", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    status = store->hset("key", "", "value");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 测试空字符串value
    status = store->hset("key", "field", "");
    ASSERT_TRUE(status.ok()) << status.error_str();

    // 验证空字符串操作
    ASSERT_TRUE(store->hexists("", "field"));
    ASSERT_TRUE(store->hexists("key", ""));
    ASSERT_TRUE(store->hexists("key", "field"));

    store->close();
}

TEST_F(RocksdbStoreTest, performance_benchmark) {
    RocksdbStorePtr store;
    auto status = RocksdbStore::open(data_path().c_str(), &store);
    ASSERT_TRUE(status.ok()) << status.error_str();

    const int num_operations = 10000;

    // 性能测试：批量写入
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_operations; ++i) {
        std::string key = fmt::format("perf_key_{}", i / 100);
        std::string field = fmt::format("field_{}", i % 100);
        std::string value = fmt::format("value_{}", i);

        status = store->hset(key, field, value);
        ASSERT_TRUE(status.ok()) << status.error_str();
    }

    auto write_end_time = std::chrono::high_resolution_clock::now();
    auto write_duration = std::chrono::duration_cast<std::chrono::milliseconds>(write_end_time - start_time).count();

    // 性能测试：批量读取
    start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_operations; ++i) {
        std::string key = fmt::format("perf_key_{}", i / 100);
        std::string field = fmt::format("field_{}", i % 100);
        std::string expected_value = fmt::format("value_{}", i);

        std::string actual_value;
        status = store->hget(key, field, &actual_value);
        ASSERT_TRUE(status.ok()) << status.error_str();
        ASSERT_EQ(actual_value, expected_value);
    }

    auto read_end_time = std::chrono::high_resolution_clock::now();
    auto read_duration = std::chrono::duration_cast<std::chrono::milliseconds>(read_end_time - start_time).count();

    // 输出性能指标
    fmt::println("Performance benchmark results:");
    fmt::println("  Write {} operations: {} ms", num_operations, write_duration);
    fmt::println("  Read {} operations: {} ms", num_operations, read_duration);
    fmt::println("  Write throughput: {:.2f} ops/ms", static_cast<double>(num_operations) / write_duration);
    fmt::println("  Read throughput: {:.2f} ops/ms", static_cast<double>(num_operations) / read_duration);

    store->close();
}

// NOLINTEND(readability-magic-numbers)
